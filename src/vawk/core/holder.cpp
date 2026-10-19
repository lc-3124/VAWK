#include "vawk/core/holder.hpp"
#include "vawk/core/entity.hpp"

#include <algorithm>
#include <utility>

namespace vawk {

//- 生命周期

Holder::~Holder() {
    // 先把本地表整表取走，避免在锁内回调实体（实体可能反过来请求放手）。
    std::vector<Held> held;
    {
        std::lock_guard lk(mtx_held_);
        held.swap(held_);
    }
    // 只持弱引用：先 lock() 拿临时强引用保命，再注销对应的 token。
    // 已经过期的条目直接跳过（实体先于持有者销毁）。
    for (auto& h : held)
        if (auto entity = h.entity.lock()) entity->remove_holder(h.token);
}

//- 登记 / 放手

std::uint64_t Holder::acquire(const std::shared_ptr<Entity>& entity, HolderKind kind) {
    if (!entity) return 0;

    // 持锁完成“查重 + 登记”，保证本地表与实体小本子成对更新。
    std::lock_guard lk(mtx_held_);
    for (auto it = held_.begin(); it != held_.end();) {
        // 顺手清掉已经过期的记录，避免同一实体地址被复用时表无限增长。
        if (it->entity.expired()) {
            it = held_.erase(it);
            continue;
        }
        if (it->entity.lock().get() == entity.get()) return it->token;
        ++it;
    }

    const auto token = entity->add_holder(kind, this);
    if (token == 0) return 0;
    held_.push_back({std::weak_ptr<Entity>(entity), token});
    return token;
}

void Holder::disown(const std::shared_ptr<Entity>& entity) {
    if (!entity) return;

    std::uint64_t token = 0;
    {
        std::lock_guard lk(mtx_held_);
        auto it = std::find_if(held_.begin(), held_.end(), [&](const Held& h) {
            return h.entity.lock().get() == entity.get();
        });
        if (it == held_.end()) return;
        token = it->token;
        if (it != held_.end() - 1) *it = std::move(held_.back());
        held_.pop_back();
    }
    // 实体注销可能触发孤儿判定与 on_disown 回调，必须在锁外调用。
    entity->remove_holder(token);
}

void Holder::disown(const Entity* entity) {
    if (!entity) return;

    std::shared_ptr<Entity> target;
    std::uint64_t           token = 0;
    {
        std::lock_guard lk(mtx_held_);
        auto it = std::find_if(held_.begin(), held_.end(), [&](const Held& h) {
            auto held = h.entity.lock();
            return held.get() == entity;
        });
        if (it == held_.end()) return;
        target = it->entity.lock();
        token  = it->token;
        if (it != held_.end() - 1) *it = std::move(held_.back());
        held_.pop_back();
    }
    // 已过期（lock() 失败）时只丢本地记录，实体已无可通知。
    if (target) target->remove_holder(token);
}

void Holder::on_disown(std::shared_ptr<Entity> entity) {
    if (!entity) return;

    // 默认放手：删掉该实体在本持有者名下的全部记录（一步到位）。实体已在
    // request_disown() 里清空小本子，这里不能再调 remove_holder()，否则会重入。
    {
        std::lock_guard lk(mtx_held_);
        auto it = std::remove_if(held_.begin(), held_.end(), [&](const Held& h) {
            return h.entity.lock().get() == entity.get();
        });
        held_.erase(it, held_.end());
    }
    // 本地只有 weak_ptr，真正的销毁由存储区松手后完成。
}

}  // namespace vawk
