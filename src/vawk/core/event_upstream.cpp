#include "vawk/core/event_upstream.hpp"

#include <utility>

namespace vawk {

//- 生命周期

EventUpstream::~EventUpstream() {
    stop_event_loop();
    // 剩余持有记录由 ~Holder() 自动注销。
}

//- 订阅索引

void EventUpstream::subscribe(std::size_t event_id, std::shared_ptr<Entity> entity) {
    if (!entity) return;

    std::lock_guard lk(mtx_listeners_);
    if (event_id >= listeners_.size()) listeners_.resize(event_id + 1);

    auto& subs = entity_events_[entity.get()];
    for (const auto& s : subs)
        if (s.event_id == event_id) return;  // 已订阅：去重

    auto&             list      = listeners_[event_id];
    const std::size_t list_slot = list.size();
    const std::size_t sub_slot  = subs.size();
    list.push_back({entity, entity.get(), sub_slot});
    subs.push_back({event_id, list_slot});

    // 订阅即持有；acquire() 内部按实体去重，重复调用不会产生多条记录。
    acquire(entity, HolderKind::Router);
}

void EventUpstream::unsubscribe(std::size_t event_id, const Entity* entity) {
    bool drop_hold = false;
    {
        std::lock_guard lk(mtx_listeners_);

        auto eit = entity_events_.find(entity);
        if (eit == entity_events_.end()) return;
        if (event_id >= listeners_.size()) return;
        auto& subs = eit->second;

        // 先在自己的订阅表里定位要删的条目。
        std::size_t sub_slot = subs.size();
        for (std::size_t i = 0; i < subs.size(); ++i)
            if (subs[i].event_id == event_id) {
                sub_slot = i;
                break;
            }
        if (sub_slot == subs.size()) return;

        // 两张表各自 swap-pop，并修正被移动者的反向槽位。
        const std::size_t list_slot = subs[sub_slot].list_slot;
        auto&             list      = listeners_[event_id];

        if (list_slot + 1 < list.size()) {
            Listener moved = list.back();
            list[list_slot] = moved;
            entity_events_[moved.raw][moved.sub_slot].list_slot = list_slot;
        }
        list.pop_back();

        if (sub_slot + 1 < subs.size()) {
            Subscription moved = subs.back();
            subs[sub_slot] = moved;
            listeners_[moved.event_id][moved.list_slot].sub_slot = sub_slot;
        }
        subs.pop_back();

        if (subs.empty()) {
            entity_events_.erase(eit);
            drop_hold = true;  // 该实体已无任何订阅，Router 不再需要持有它
        }
    }
    // 锁外释放：disown 会通知实体注销，可能触发实体释放流程。
    if (drop_hold) disown(entity);
}

void EventUpstream::unsubscribe_all(const Entity* entity) {
    // 每轮重新取锁：unsubscribe() 内部还要加锁，不能持锁递归。
    for (;;) {
        std::size_t id = 0;
        {
            std::lock_guard lk(mtx_listeners_);
            auto it = entity_events_.find(entity);
            if (it == entity_events_.end() || it->second.empty()) return;
            id = it->second.front().event_id;
        }
        unsubscribe(id, entity);
    }
}

void EventUpstream::detach(const Entity* entity) {
    unsubscribe_all(entity);
    disown(entity);
}

std::vector<std::shared_ptr<Entity>>
EventUpstream::subscribers(std::size_t event_id) const {
    return snapshot_listeners(event_id);
}

void EventUpstream::on_disown(std::shared_ptr<Entity> entity) {
    if (!entity) return;

    // 先摘订阅：索引表只存 weak_ptr，但残留条目仍会参与派发与状态展示，
    // 放手时必须一并清掉。
    unsubscribe_all(entity.get());

    // 默认放手删掉本持有者名下的全部记录，不再回头动实体（避免重入）。
    Holder::on_disown(std::move(entity));
}

std::vector<std::shared_ptr<Entity>>
EventUpstream::snapshot_listeners(std::size_t event_id) const {
    std::lock_guard lk(mtx_listeners_);

    std::vector<std::shared_ptr<Entity>> result;
    if (event_id >= listeners_.size()) return result;

    result.reserve(listeners_[event_id].size());
    for (const auto& l : listeners_[event_id]) {
        // 已过期的条目直接跳过；MVP 不做惰性清理（方法保持 const）。
        if (auto entity = l.entity.lock()) result.push_back(std::move(entity));
    }
    return result;
}

//- 生命周期

void EventUpstream::start_event_loop() {
    std::lock_guard lk(mtx_lifecycle_);
    if (running_.load()) return;
    running_.store(true);
    event_thread_ = std::thread([this] { dispatch_loop(); });
}

void EventUpstream::stop_event_loop() {
    {
        std::lock_guard lk(mtx_lifecycle_);
        if (!running_.exchange(false)) return;
    }

    // 先置标志再持子类队列锁唤醒，确保循环一定看到 running_ == false。
    wake_dispatch_thread();

    if (!event_thread_.joinable()) return;
    if (event_thread_.get_id() == std::this_thread::get_id())
        event_thread_.detach();  // 事件线程内调用：不能自我 join
    else
        event_thread_.join();
}

}  // namespace vawk
