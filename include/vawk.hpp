#ifndef VAWK_HPP
#define VAWK_HPP

// -----------------------------------------------------------------------
//  vawk — Visual Ansi Widget Kit (core event framework)
//
//  Provides an event-driven framework for building TUI applications on
//  top of the vaterm terminal library.
//
//  Architecture overview:
//
//    vawk::event::Base   <──  User-defined event structs
//         │                       (VAWK_EVENT_DEFINE macro)
//         │
//         ├── Entity            ──  Receives events (override on_event)
//         │
//         ├── EventUpstream     ──  Source of events, routes to entities
//         │       │
//         │       └── EventRouter  Concrete upstream with push-able buffer
//         │
//         └── Input (placeholder)  Reads terminal input → events
//
//  Typical usage:
//    1. Define your event types with VAWK_EVENT_DEFINE / VAWK_EVENT_DEFINE_END.
//    2. Create an EventRouter as the central event bus.
//    3. Subclass Entity, override on_event(), and subscribe to events.
//    4. Push events into the router; entities receive them via dispatch.
//
//  Example:
//    #include <vawk.hpp>
//
//    VAWK_EVENT_DEFINE(MyEvent) int value; VAWK_EVENT_DEFINE_END
//
//    class MyEntity : public vawk::Entity {
//      void on_event(std::shared_ptr<vawk::event::Base> evt) override {
//          if (auto* e = vawk::event::cast<MyEvent>(*evt)) {
//              printf("got value %d\n", e->value);
//          }
//      }
//    };
// -----------------------------------------------------------------------

#include "vawk/event.hpp"
#include "vawk/entity.hpp"
#include "vawk/event_upstream.hpp"
#include "vawk/event_router.hpp"
#include "vawk/input.hpp"
#include "vawk/widget.hpp"
#include "vawk/window.hpp"
#include "vawk/drawable.hpp"

#endif  // VAWK_HPP
