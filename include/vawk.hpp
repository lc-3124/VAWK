#ifndef VAWK_HPP
#define VAWK_HPP

/// vawk — Visual Ansi Widget Kit (core framework)
///
/// Provides an event-driven framework for building TUI applications.
/// Depends on the vaterm library for low-level terminal I/O.
///
/// Modules:
///   vawk::event       — event type system (Base, is<T>, cast<T>)
///   vawk::Entity      — event-receiving entity base class
///   vawk::EventUpstream — event source base class
///   vawk::EventRouter  — event router with push-able buffer
///   vawk::Input        — terminal input handler (placeholder)
///
/// Example:
///   #include <vawk.hpp>
///
///   VAWK_EVENT_DEFINE(MyEvent) int value; VAWK_EVENT_DEFINE_END
///
///   class MyEntity : public vawk::Entity {
///     void on_event(std::shared_ptr<vawk::event::Base> evt) override {
///         if (auto* e = vawk::event::cast<MyEvent>(*evt)) {
///             // handle e->value
///         }
///     }
///   };

#include "vawk/event.hpp"
#include "vawk/entity.hpp"
#include "vawk/event_upstream.hpp"
#include "vawk/event_router.hpp"
#include "vawk/input.hpp"
#include "vawk/widget.hpp"
#include "vawk/window.hpp"
#include "vawk/drawable.hpp"

#endif  // VAWK_HPP

