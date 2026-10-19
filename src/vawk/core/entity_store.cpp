#include "vawk/core/entity_store.hpp"
#include "vawk/core/entity.hpp"

#include <iterator>
#include <memory>
#include <utility>

namespace vawk {
namespace detail {

EntityStore& EntityStore::instance() {
    static EntityStore store;
    return store;
}

EntityStore::~EntityStore() = default;

void EntityStore::attach(const std::shared_ptr<Entity>& entity) {
    if (!entity) return;

    std::lock_guard lk(mtx_);
    if (entity->in_store_) return;  // 重复 attach 幂等
    items_.push_back(entity);
    entity->store_pos_ = std::prev(items_.end());
    entity->in_store_  = true;
}

void EntityStore::release(Entity* entity) {
    if (!entity) return;

    // 先摘下强引用，出锁后再让它析构：实体析构可能回调 upstream / Holder，
    // 在锁内做会带来反向加锁与死锁风险。
    std::shared_ptr<Entity> doomed;
    {
        std::lock_guard lk(mtx_);
        if (!entity->in_store_) return;
        doomed = std::move(*entity->store_pos_);
        items_.erase(entity->store_pos_);
        entity->in_store_ = false;
    }
    // doomed 在此析构；若这是最后一份强引用，实体随即销毁。
}

}  // namespace detail
}  // namespace vawk
