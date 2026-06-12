#ifndef VAWK_ENTITY_HPP
#define VAWK_ENTITY_HPP

#include "event.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace vawk {

// Forward declaration
class EventUpstream;

/// Base class for all event-receiving entities.
///
/// Entity provides a thread-safe event buffer and a virtual interface for
/// handling events.  Users should inherit from Entity and override
/// `on_event()` to define custom event logic.
///
/// Unlike the original VaEntity, this version does NOT call
/// `shared_from_this()` in subscribe/unsubscribe, avoiding the
/// `std::bad_weak_ptr` pitfall.  Subscription is done via raw pointer;
/// the caller is responsible for ensuring the entity outlives the upstream.
class Entity {
  public:
    Entity() = default;
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    virtual ~Entity();

    // ── Event buffer ────────────────────────────────────────────────────

    /// Push an event into the buffer.  Thread-safe.
    void push_event(std::shared_ptr<event::Base> evt);

    /// Process one event from the buffer (FIFO).
    /// Returns  1 on success (event was handled),
    ///          0 if the event was null,
    ///         -1 if the buffer was empty.
    int process_one_event();

    // ── Subscription helpers ────────────────────────────────────────────

    /// Subscribe to one or more event types from an upstream.
    /// @note  Does NOT use shared_from_this().  The upstream stores a
    ///        raw pointer; ensure this entity outlives the upstream.
    void subscribe(EventUpstream* upstream, std::vector<size_t> event_ids);

    /// Unsubscribe from specific event types.
    void unsubscribe(EventUpstream* upstream, std::vector<size_t> event_ids);

    /// Unsubscribe from all event types on the given upstream.
    void unsubscribe(EventUpstream* upstream);

    // ── Label ───────────────────────────────────────────────────────────

    std::string label() const { return label_; }
    void set_label(std::string l) { label_ = std::move(l); }

  protected:
    /// Override this to handle events.  Called by push_event() and
    /// process_one_event().
    virtual void on_event(std::shared_ptr<event::Base> evt) = 0;

    // ── Internal state ─────────────────────────────────────────────────

    std::mutex mutex_;
    std::queue<std::shared_ptr<event::Base>> event_buffer_;

    std::vector<EventUpstream*> upstreams_;
    std::string label_;
};

}  // namespace vawk

#endif  // VAWK_ENTITY_HPP

