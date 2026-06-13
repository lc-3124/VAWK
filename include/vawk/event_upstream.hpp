#ifndef VAWK_EVENT_UPSTREAM_HPP
#define VAWK_EVENT_UPSTREAM_HPP

#include "entity.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace vawk {

/// Base class for event sources that dispatch events to registered entities.
///
/// Maintains two indexes:
///   - listeners_  : per-event-type list of raw Entity pointers
///   - entity_events_ : per-entity list of registered event type IDs
///
/// Entities are stored as raw pointers (not shared_ptr/weak_ptr) to avoid
/// the `shared_from_this()` lifetime issues present in the original design.
/// The caller must ensure that entities are properly unregistered before
/// they are destroyed.
class EventUpstream {
  public:
    EventUpstream() = default;
    virtual ~EventUpstream();

    // ── Registration ────────────────────────────────────────────────────

    /// Register an entity to receive events of the given type.
    void register_listener(size_t event_id, Entity* entity);

    /// Unregister an entity from all event types.
    void unregister_listener(Entity* entity);

    /// Unregister all entities from a specific event type.
    void unregister_listener(size_t event_id);

    /// Unregister a specific entity from a specific event type.
    void unregister_listener(Entity* entity, size_t event_id);

    // ── Dispatch ────────────────────────────────────────────────────────

    /// Dispatch one event from the buffer (pure virtual).
    virtual void dispatch_once() = 0;

    /// Event loop running in a dedicated thread (pure virtual).
    virtual void dispatch_loop() = 0;

    // ── Lifecycle ───────────────────────────────────────────────────────

    void start_event_loop();
    void stop_event_loop();
    bool is_running() const { return running_; }

  protected:
    std::mutex mutex_;
    std::mutex cv_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::thread event_thread_;

    // listeners_[event_id] = list of entities
    std::vector<std::vector<Entity*>> listeners_;
    // entity_events_[entity] = list of event type IDs
    std::unordered_map<Entity*, std::vector<size_t>> entity_events_;
};

}  // namespace vawk

#endif  // VAWK_EVENT_UPSTREAM_HPP

