#ifndef VAWK_EVENT_UPSTREAM_HPP
#define VAWK_EVENT_UPSTREAM_HPP

// -----------------------------------------------------------------------
//  vawk::EventUpstream — event source base class
//
//  An EventUpstream is any component that produces events and delivers
//  them to registered Entity instances.  It maintains two indexes:
//
//    listeners_[event_id]  → list of Entity* that want that event type
//    entity_events_[entity] → list of event type IDs the entity is
//                             registered for (for bulk unregister)
//
//  Entities are stored as raw pointers.  The caller must ensure every
//  Entity is unregistered before it is destroyed.
//
//  Subclasses must implement dispatch_once() and dispatch_loop().
//  The base class provides start_event_loop() / stop_event_loop() to
//  run dispatch_loop() in a background thread.
// -----------------------------------------------------------------------

#include "entity.hpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace vawk {

class EventUpstream {
  public:
    EventUpstream() = default;
    virtual ~EventUpstream();

    // ── Registration ────────────────────────────────────────────────────

    // Register an entity to receive events of the given type ID.
    void register_listener(size_t event_id, Entity* entity);

    // Unregister an entity from all event types it is subscribed to.
    void unregister_listener(Entity* entity);

    // Unregister all entities from a specific event type.
    void unregister_listener(size_t event_id);

    // Unregister a specific entity from a specific event type.
    void unregister_listener(Entity* entity, size_t event_id);

    // ── Dispatch (pure virtual) ─────────────────────────────────────────

    // Dispatch exactly one event from the source to its listeners.
    virtual void dispatch_once() = 0;

    // Background event loop.  Runs continuously, calling dispatch_once()
    // in a tight loop.  Used by start_event_loop() in a dedicated thread.
    virtual void dispatch_loop() = 0;

    // ── Lifecycle ───────────────────────────────────────────────────────

    // Start the event loop in a background thread.
    void start_event_loop();

    // Signal the event loop to stop and join the thread.
    void stop_event_loop();

    // Check whether the event loop thread is currently running.
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
