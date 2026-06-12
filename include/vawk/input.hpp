#ifndef VAWK_INPUT_HPP
#define VAWK_INPUT_HPP

#include "event_upstream.hpp"

namespace vawk {

// Forward declarations for input event types.
VAWK_EVENT_DEFINE(MouseClickEvent)
    float x, y;
    int   duration = -1;
VAWK_EVENT_DEFINE_END

/// Handles terminal input (mouse, keyboard) and translates it into events.
///
/// This is a placeholder — the implementation is not yet complete.
class Input {
  public:
    Input() = default;
    ~Input() = default;

    void start();
    void stop();

  private:
    bool running_ = false;
};

}  // namespace vawk

#endif  // VAWK_INPUT_HPP

