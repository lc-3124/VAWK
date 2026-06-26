#include "vawk/event_upstream.hpp"

#include <algorithm>

namespace vawk {

EventUpstream::~EventUpstream() {
    stop_event_loop();
}

void EventUpstream::register_listener(size_t event_id, Entity* entity) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (event_id >= listeners_.size())
        listeners_.resize(event_id + 1);
    listeners_[event_id].push_back(entity);
    entity_events_[entity].push_back(event_id);
}

void EventUpstream::unregister_listener(Entity* entity) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entity_events_.find(entity);
    if (it == entity_events_.end()) return;
    for (auto id : it->second) {
        if (id < listeners_.size()) {
            auto& vec = listeners_[id];
            vec.erase(std::remove(vec.begin(), vec.end(), entity), vec.end());
        }
    }
    entity_events_.erase(it);
}

void EventUpstream::unregister_listener(size_t event_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (event_id >= listeners_.size()) return;
    auto& vec = listeners_[event_id];
    for (auto* entity : vec) {
        auto it = entity_events_.find(entity);
        if (it != entity_events_.end()) {
            auto& ids = it->second;
            ids.erase(std::remove(ids.begin(), ids.end(), event_id), ids.end());
            if (ids.empty())
                entity_events_.erase(it);
        }
    }
    vec.clear();
}

void EventUpstream::unregister_listener(Entity* entity, size_t event_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (event_id < listeners_.size()) {
        auto& vec = listeners_[event_id];
        vec.erase(std::remove(vec.begin(), vec.end(), entity), vec.end());
    }
    auto it = entity_events_.find(entity);
    if (it != entity_events_.end()) {
        auto& ids = it->second;
        ids.erase(std::remove(ids.begin(), ids.end(), event_id), ids.end());
        if (ids.empty())
            entity_events_.erase(it);
    }
}

void EventUpstream::start_event_loop() {
    {
        std::lock_guard<std::mutex> lock(cv_mutex_);
        if (running_) return;
        running_ = true;
    }
    event_thread_ = std::thread([this] { dispatch_loop(); });
}

void EventUpstream::stop_event_loop() {
    {
        std::lock_guard<std::mutex> lock(cv_mutex_);
        if (!running_) return;
        running_ = false;
    }
    cv_.notify_all();
    if (event_thread_.joinable())
        event_thread_.join();
}

} // namespace vawk
