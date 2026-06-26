#include "vawk/entity.hpp"
#include "vawk/event_upstream.hpp"

#include <algorithm>

namespace vawk {

Entity::~Entity() {
    for (auto* upstream : upstreams_) {
        if (upstream)
            upstream->unregister_listener(this);
    }
}

void Entity::push_event(std::shared_ptr<event::Base> evt) {
    std::lock_guard<std::mutex> lock(mutex_);
    event_buffer_.push(std::move(evt));
}

Entity::ProcessResult Entity::process_one_event() {
    std::shared_ptr<event::Base> evt;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (event_buffer_.empty())
            return Entity::ProcessResult::Empty;
        evt = std::move(event_buffer_.front());
        event_buffer_.pop();
    }
    if (!evt)
        return Entity::ProcessResult::Discarded;
    on_event(std::move(evt));
    return Entity::ProcessResult::Processed;
}

void Entity::subscribe(EventUpstream* upstream, std::vector<size_t> event_ids) {
    if (!upstream) return;
    for (auto id : event_ids)
        upstream->register_listener(id, this);
    if (std::find(upstreams_.begin(), upstreams_.end(), upstream) == upstreams_.end())
        upstreams_.push_back(upstream);
}

void Entity::unsubscribe(EventUpstream* upstream, std::vector<size_t> event_ids) {
    if (!upstream) return;
    for (auto id : event_ids)
        upstream->unregister_listener(this, id);
}

void Entity::unsubscribe(EventUpstream* upstream) {
    if (!upstream) return;
    upstream->unregister_listener(this);
}

} // namespace vawk
