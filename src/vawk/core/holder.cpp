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
    for (auto& h : held)
        if (h.entity) h.entity->remove_holder(h.token);
}

//- 登记 / 放手

std::uint64_t Holder::acquire(const std::shared_ptr<Entity>& entity, HolderKind kind) {
    if (!entity) return 0;

    // 持锁完成“查重 + 登记”，保证本地表与实体小本子成对更新。
    std::lock_guard lk(mtx_held_);
    for (const auto& h : held_)
        if (h.entity == entity) return h.token;

    const auto token = entity->add_holder(kind, this);
    if (token == 0) return 0;
    held_.push_back({entity, token});
    return token;
}

void Holder::release(const std::shared_ptr<Entity>& entity) {
    if (!entity) return;

    std::uint64_t token = 0;
    {
        std::lock_guard lk(mtx_held_);
        auto it = std::find_if(held_.begin(), held_.end(),
                               [&](const Held& h) { return h.entity == entity; });
        if (it == held_.end()) return;
        token = it->token;
        if (it != held_.end() - 1) *it = std::move(held_.back());
        held_.pop_back();
    }
    // 实体注销可能触发孤儿判定与 holder_release 回调，必须在锁外调用。
    entity->remove_holder(token);
}

void Holder::release_by_ptr(const Entity* entity) {
    if (!entity) return;

    std::shared_ptr<Entity> target;
    std::uint64_t           token = 0;
    {
        std::lock_guard lk(mtx_held_);
        auto it = std::find_if(held_.begin(), held_.end(),
                               [&](const Held& h) { return h.entity.get() == entity; });
        if (it == held_.end()) return;
        target = it->entity;
        token  = it->token;
        if (it != held_.end() - 1) *it = std::move(held_.back());
        held_.pop_back();
    }
    if (target) target->remove_holder(token);
}

void Holder::holder_release(std::shared_ptr<Entity> entity) {
    if (!entity) return;

    // 默认放手：删掉该实体在本持有者名下的全部记录（一步到位）。实体已在
    // request_release() 里清空小本子，这里不能再调 remove_holder()，否则会重入。
    // 被移除的 shared_ptr 先收集，锁外析构，避免在锁内触发实体析构。
    std::vector<std::shared_ptr<Entity>> doomed;
    {
        std::lock_guard lk(mtx_held_);
        auto it = std::remove_if(held_.begin(), held_.end(), [&](const Held& h) {
            if (h.entity == entity) {
                doomed.push_back(h.entity);
                return true;
            }
            return false;
        });
        held_.erase(it, held_.end());
    }
    // doomed 在此析构；若这是最后一份强引用，实体随之销毁。
}

}  // namespace vawk
