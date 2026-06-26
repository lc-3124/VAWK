#include "vawk/event_router.hpp"

namespace vawk {

bool EventRouter::push(std::shared_ptr<event::Base> evt) {
    std::lock_guard<std::mutex> lock(mutex_);
    event_buffer_.push(std::move(evt));
    cv_.notify_one();
    return true;
}

void EventRouter::dispatch_once() {
    std::shared_ptr<event::Base> evt;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (event_buffer_.empty())
            return;
        evt = std::move(event_buffer_.front());
        event_buffer_.pop();
    }
    if (!evt) return;

    size_t eid = evt->id();
    std::vector<Entity*> targets;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (eid < listeners_.size())
            targets = listeners_[eid];
    }

    for (auto* entity : targets) {
        if (entity)
            entity->push_event(evt);
    }
}

void EventRouter::dispatch_loop() {
    while (true) {
        if (!running_ && event_buffer_.empty())
            break;
        dispatch_once();
        std::this_thread::yield();
    }
}

} // namespace vawk
