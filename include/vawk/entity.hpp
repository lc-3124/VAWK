#ifndef VAWK_ENTITY_HPP
#define VAWK_ENTITY_HPP

// -----------------------------------------------------------------------
//  vawk::Entity — event-receiving entity base class
//
//  Entity provides:
//    - A thread-safe event buffer (push_event / process_one_event).
//    - Convenience subscription helpers (subscribe / unsubscribe).
//    - An optional human-readable label (set_label / label).
//
//  Subclass Entity and override the protected on_event() method to
//  handle incoming events.
//
//  Subscription is done via raw pointer to EventUpstream (not via
//  shared_from_this) to avoid the std::bad_weak_ptr pitfall present
//  in the original design.  The caller must ensure the Entity outlives
//  the upstreams it is subscribed to.
// -----------------------------------------------------------------------

#include "event.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace vawk {

// Forward declaration — defined in event_upstream.hpp.
class EventUpstream;

class Entity {
  public:
    Entity() = default;
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    virtual ~Entity();

    // ── Event buffer ────────────────────────────────────────────────────

    // Push an event into the thread-safe internal queue.
    // The event will later be delivered via on_event().
    void push_event(std::shared_ptr<event::Base> evt);

    // Dequeue and deliver one event.
    //
    // Returns:
    //    1  — event was dequeued and delivered
    //    0  — event was null (discarded)
    //   -1  — buffer was empty
    int process_one_event();

    // ── Subscription helpers ────────────────────────────────────────────

    // Subscribe to one or more event type IDs from an upstream source.
    void subscribe(EventUpstream* upstream, std::vector<size_t> event_ids);

    // Unsubscribe from specific event type IDs on an upstream.
    void unsubscribe(EventUpstream* upstream, std::vector<size_t> event_ids);

    // Unsubscribe from all event types on the given upstream.
    void unsubscribe(EventUpstream* upstream);

    // ── Label ───────────────────────────────────────────────────────────

    std::string label() const { return label_; }
    void set_label(std::string l) { label_ = std::move(l); }

  protected:
    // Override this to handle events pushed into the buffer.
    virtual void on_event(std::shared_ptr<event::Base> evt) = 0;

    // ── Internal state ─────────────────────────────────────────────────

    std::mutex mutex_;
    std::queue<std::shared_ptr<event::Base>> event_buffer_;

    // List of upstreams this entity is subscribed to (for bulk
    // unsubscribe).
    std::vector<EventUpstream*> upstreams_;
    std::string label_;
};

}  // namespace vawk

#endif  // VAWK_ENTITY_HPP
