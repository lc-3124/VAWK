#ifndef VAWK_EVENT_ROUTER_HPP
#define VAWK_EVENT_ROUTER_HPP

// -----------------------------------------------------------------------
//  vawk::EventRouter — concrete EventUpstream with push-able buffer
//
//  EventRouter is the simplest EventUpstream implementation: events are
//  pushed into an internal queue, and dispatch_once() dequeues and
//  delivers them to registered listeners.
//
//  Typical use:
//    - Create a single EventRouter as the application event bus.
//    - Entities subscribe to the router for the event types they care
//      about.
//    - Any part of the code can push() events into the router.
//    - Either call dispatch_once() manually on the main thread, or
//      use start_event_loop() for background delivery.
// -----------------------------------------------------------------------

#include "event_upstream.hpp"

#include <memory>
#include <queue>

namespace vawk {

class EventRouter : public EventUpstream {
  public:
    EventRouter() = default;
    ~EventRouter() override { stop_event_loop(); }

    // Push a new event into the internal buffer.
    // Returns true on success (currently always true).
    bool push(std::shared_ptr<event::Base> evt);

    // Dequeue one event and dispatch it to all registered listeners
    // for that event's type.
    void dispatch_once() override;

    // Background event loop: repeatedly calls dispatch_once().
    // Stopped via stop_event_loop().
    void dispatch_loop() override;

  protected:
    std::queue<std::shared_ptr<event::Base>> event_buffer_;
};

}  // namespace vawk

#endif  // VAWK_EVENT_ROUTER_HPP
