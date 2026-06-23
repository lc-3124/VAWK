#ifndef VAWK_EVENT_HPP
#define VAWK_EVENT_HPP

// -----------------------------------------------------------------------
//  vawk::event — event type system
//
//  Every event inherits from vawk::event::Base, which provides a virtual
//  id() method returning a per-type unique size_t.  Type IDs are
//  generated at runtime via a global atomic counter, triggered on first
//  access to event_type_id<T>().
//
//  Macros:
//    VAWK_EVENT_DEFINE(Name) ... VAWK_EVENT_DEFINE_END
//        Defines a struct Name : Base with the correct id() override.
//
//  Free functions:
//    event::is<T>(Base&)   — true if the event is of type T
//    event::cast<T>(Base&) — downcast, or nullptr on mismatch
// -----------------------------------------------------------------------

#include <atomic>
#include <cstddef>
#include <cstdlib>

namespace vawk {

// ---- Type-ID generation -----------------------------------------------

// Global counter for assigning unique IDs to event types.
inline std::atomic<size_t>& global_event_id_counter() {
    static std::atomic<size_t> counter{0};
    return counter;
}

// Return the unique compile-time-per-invocation type ID for T.
// Each instantiation gets the next counter value at program startup
// (guaranteed by the function-local static initialiser).
template <typename T>
size_t event_type_id() {
    static size_t id = global_event_id_counter()++;
    return id;
}

// ---- Macros for defining event types ----------------------------------

// Injects the required id() override into a struct that inherits from
// vawk::event::Base.
#define VAWK_EVENT_BASE_METHOD(T)      \
    size_t id() const override {       \
        return event_type_id<T>();     \
    }

// Define a new event type. Usage:
//
//   VAWK_EVENT_DEFINE(MouseEvent)
//       int x, y;
//   VAWK_EVENT_DEFINE_END
#define VAWK_EVENT_DEFINE(EventName)                \
    struct EventName : public vawk::event::Base {   \
        VAWK_EVENT_BASE_METHOD(EventName)

#define VAWK_EVENT_DEFINE_END \
    };

// ---- event namespace --------------------------------------------------

namespace event {

// Base class for all events in the system.
// Every concrete event type must inherit from Base and override id().
struct Base {
    virtual ~Base() = default;
    virtual size_t id() const = 0;
};

// Runtime type check: is e an instance of T?
template <typename T>
bool is(const Base& e) {
    return e.id() == event_type_id<T>();
}

// Downcast: if e is T, return a pointer to the T portion; else nullptr.
template <typename T>
const T* cast(const Base& e) {
    if (is<T>(e)) {
        return &static_cast<const T&>(e);
    }
    return nullptr;
}

}  // namespace event
}  // namespace vawk

#endif  // VAWK_EVENT_HPP
