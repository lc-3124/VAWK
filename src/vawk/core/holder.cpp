#include "vawk/core/holder.hpp"
#include "vawk/core/entity.hpp"

#include <algorithm>
#include <utility>

namespace vawk {

//- 生命周期

Holder::~Holder() {
    // 先把本地表整表取走，避免在锁内回调实体（实体可能反过来请求放手）。
    std::vector<std::weak_ptr<Entity>> held;
    {
        std::lock_guard lk(mtx_held_);
        held.reserve(held_.size());
        for (auto& h : held_) held.push_back(h.entity);
        held_.clear();
    }

    // 本地表已空，逐实体只锁它的小本子即可。只持弱引用：lock() 成功的
    // 临时强引用保证注销期间实体不会消失；已过期的条目直接跳过。
    for (auto& w : held) {
        auto e = w.lock();
        if (!e) continue;

        bool orphan = false;
        {
            std::scoped_lock lk(e->mtx_holders_);
            auto it = std::find_if(e->holders_.begin(), e->holders_.end(),
                                   [&](const Entity::HolderEntry& h) { return h.owner == this; });
            if (it == e->holders_.end()) continue;
            if (it != e->holders_.end() - 1) *it = std::move(e->holders_.back());
            e->holders_.pop_back();
            if (e->enabled_.load())
                orphan = e->release_policy_(e->build_holder_info_locked());
        }
        // 回调会摸别的 holder，必须在锁外。
        if (orphan) e->request_disown();
    }
}

//- 登记 / 放手

void Holder::acquire(const std::shared_ptr<Entity>& entity, HolderKind kind) {
    if (!entity) return;

    // 一把双锁同时覆盖两本账：查重、登记都是原子的。
    std::scoped_lock lk(mtx_held_, entity->mtx_holders_);

    for (auto it = held_.begin(); it != held_.end();) {
        // 顺手清掉已经过期的记录，避免同一实体地址复用时表无限增长。
        if (it->entity.expired()) {
            it = held_.erase(it);
            continue;
        }
        if (it->entity.lock() == entity) return;  // 去重
        ++it;
    }

    entity->holders_.push_back({kind, this});
    held_.push_back({entity});
}

void Holder::disown(const std::shared_ptr<Entity>& entity) {
    if (!entity) return;

    bool orphan = false;
    {
        std::scoped_lock lk(mtx_held_, entity->mtx_holders_);
        disown_locked_(entity.get(), orphan);
    }
    // 实体注销可能触发孤儿判定与 on_disown 回调，必须在锁外调用。
    if (orphan) entity->request_disown();
}

void Holder::disown(const Entity* entity) {
    if (!entity) return;

    // 先用弱引用取一份临时强引用：传入的裸指针可能已经过期，直接对它加锁
    // 会碰到已析构的 mutex。lock() 成功同时说明实体仍然存活。
    std::shared_ptr<Entity> target;
    {
        std::lock_guard lk(mtx_held_);
        auto it = std::find_if(held_.begin(), held_.end(), [&](const Held& h) {
            return h.entity.lock().get() == entity;
        });
        if (it == held_.end()) return;
        target = it->entity.lock();
        if (!target) {
            // 实体已销毁：只丢本地垃圾记录，无账可对。
            if (it != held_.end() - 1) *it = std::move(held_.back());
            held_.pop_back();
            return;
        }
    }

    bool orphan = false;
    {
        std::scoped_lock lk(mtx_held_, target->mtx_holders_);
        disown_locked_(target.get(), orphan);
    }
    if (orphan) target->request_disown();
}

void Holder::disown_locked_(Entity* entity, bool& orphan) {
    // 1) 删本地记录。
    auto it = std::find_if(held_.begin(), held_.end(), [&](const Held& h) {
        return h.entity.lock().get() == entity;
    });
    if (it == held_.end()) return;
    if (it != held_.end() - 1) *it = std::move(held_.back());
    held_.pop_back();

    // 2) 删实体小本子上属于本持有者的条目。
    auto eit = std::find_if(entity->holders_.begin(), entity->holders_.end(),
                            [&](const Entity::HolderEntry& h) { return h.owner == this; });
    if (eit != entity->holders_.end()) {
        if (eit != entity->holders_.end() - 1) *eit = std::move(entity->holders_.back());
        entity->holders_.pop_back();
    }

    // 3) 已启用才判定；结果由调用方在锁外收尾。
    if (entity->enabled_.load())
        orphan = entity->release_policy_(entity->build_holder_info_locked());
}

void Holder::on_disown(std::shared_ptr<Entity> entity) {
    if (!entity) return;

    // 默认放手：删掉该实体在本持有者名下的全部记录（一步到位）。实体已在
    // request_disown() 里清空小本子，这里不能再动它的小本子，否则会重入。
    std::lock_guard lk(mtx_held_);
    auto it = std::remove_if(held_.begin(), held_.end(), [&](const Held& h) {
        return h.entity.lock() == entity;
    });
    held_.erase(it, held_.end());
    // 本地只有 weak_ptr，真正的销毁由存储区松手后完成。
}

}  // namespace vawk
