#ifndef VAWK_EVENT_ROUTER_HPP
#define VAWK_EVENT_ROUTER_HPP

#include "event_upstream.hpp"

#include <memory>
#include <queue>

namespace vawk {

/// An EventUpstream that also provides a push-able event buffer.
///
/// Events are pushed into the buffer and then dispatched to registered
/// listeners via dispatch_once() or the background event loop.
class EventRouter : public EventUpstream {
  public:
    EventRouter() = default;
    ~EventRouter() override { stop_event_loop(); }

    /// Push an event into the buffer.  Returns true on success.
    bool push(std::shared_ptr<event::Base> evt);

    /// Dispatch one event from the buffer to its listeners.
    void dispatch_once() override;

    /// Background event loop (runs in a dedicated thread).
    void dispatch_loop() override;

  protected:
    std::queue<std::shared_ptr<event::Base>> event_buffer_;
};

}  // namespace vawk

#endif  // VAWK_EVENT_ROUTER_HPP

