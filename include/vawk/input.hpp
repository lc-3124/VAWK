#ifndef VAWK_INPUT_HPP
#define VAWK_INPUT_HPP

// -----------------------------------------------------------------------
//  vawk::Input — terminal input handler (placeholder / stub)
//
//  This module is intended to bridge vaterm's low-level terminal I/O
//  (mouse::capture, terminal::read_byte, etc.) into the vawk event
//  system.
//
//  Currently defined:
//    - MouseClickEvent  — event type for mouse click/drag events
//
//  Still to implement:
//    - Keyboard events (key press / release)
//    - Terminal resize events
//    - Actual input reading loop that feeds Entity subscribers
//
//  The Input class below is a stub; start() and stop() do nothing yet.
// -----------------------------------------------------------------------

#include "event_upstream.hpp"

#include <optional>

namespace vawk {

// Forward-declared event types.

// Emitted when a mouse button is pressed, released, or dragged.
VAWK_EVENT_DEFINE(MouseClickEvent)
    float x, y;                   // normalised or cell coordinates
    std::optional<int> duration;  // time since last press (ms), nullopt = unknown
VAWK_EVENT_DEFINE_END

// Input handler.
// This is a placeholder — the implementation is not yet complete.
class Input {
  public:
    Input() = default;
    ~Input() = default;

    // Start listening for input events.
    void start();

    // Stop listening for input events.
    void stop();

  private:
    bool running_ = false;
};

}  // namespace vawk

#endif  // VAWK_INPUT_HPP
