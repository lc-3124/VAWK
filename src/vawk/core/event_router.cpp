#include "vawk/core/event_router.hpp"

#include <utility>

namespace vawk {

//- 入队

bool EventRouter::push(std::shared_ptr<event::Base> evt) {
    if (!evt) return false;
    {
        std::lock_guard lk(mtx_router_queue_);
        event_buffer_.push_back(std::move(evt));
        // 持锁通知：消费线程可能正好在“判空 → 阻塞”之间，锁外通知会丢。
        cv_router_queue_.notify_one();
    }
    return true;
}

//- 派发

void EventRouter::dispatch_event(std::shared_ptr<event::Base> evt) {
    // 锁内快照、锁外投递：push_event() 可能阻塞，不能占着订阅索引锁。
    auto targets = snapshot_listeners(evt->id());
    for (auto& e : targets)
        if (e) e->push_event(evt);
}

void EventRouter::dispatch_once() {
    std::shared_ptr<event::Base> evt;
    {
        std::lock_guard lk(mtx_router_queue_);
        if (event_buffer_.empty()) return;
        evt = std::move(event_buffer_.front());
        event_buffer_.pop_front();
    }
    dispatch_event(std::move(evt));
}

void EventRouter::dispatch_loop() {
    for (;;) {
        std::shared_ptr<event::Base> evt;
        {
            std::unique_lock lk(mtx_router_queue_);
            cv_router_queue_.wait(lk, [&] {
                return !event_buffer_.empty() || !running_.load();
            });
            // 停止后把剩余事件排空再退出。
            if (!running_.load() && event_buffer_.empty()) break;
            evt = std::move(event_buffer_.front());
            event_buffer_.pop_front();
        }
        dispatch_event(std::move(evt));
    }
}

void EventRouter::wake_dispatch_thread() {
    // 持锁通知，与 dispatch_loop() 的谓词检查互斥。
    std::lock_guard lk(mtx_router_queue_);
    cv_router_queue_.notify_all();
}

}  // namespace vawk
