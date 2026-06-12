#ifndef VAWK_EVENT_HPP
#define VAWK_EVENT_HPP

#include <atomic>
#include <cstddef>
#include <cstdlib>

namespace vawk {

/// Returns a global atomic counter for generating unique event type IDs.
inline std::atomic<size_t>& global_event_id_counter() {
    static std::atomic<size_t> counter{0};
    return counter;
}

/// Returns a unique compile-time-per-invocation type ID for event type T.
template <typename T>
size_t event_type_id() {
    static size_t id = global_event_id_counter()++;
    return id;
}

/// Macro: inject the required `id()` override into an event struct.
#define VAWK_EVENT_BASE_METHOD(T)      \
    size_t id() const override {       \
        return event_type_id<T>();     \
    }

/// Macro: define a new event type that inherits from vawk::event::Base.
///
/// Usage:
///   VAWK_EVENT_DEFINE(MouseEvent)
///       int x, y;
///   VAWK_EVENT_DEFINE_END
#define VAWK_EVENT_DEFINE(EventName)                \
    struct EventName : public vawk::event::Base {   \
        VAWK_EVENT_BASE_METHOD(EventName)

#define VAWK_EVENT_DEFINE_END \
    };

namespace event {

/// Base class for all events. Every event type must inherit from this.
struct Base {
    virtual ~Base() = default;
    virtual size_t id() const = 0;
};

/// Check whether an event is of type T at runtime.
template <typename T>
bool is(const Base& e) {
    return e.id() == event_type_id<T>();
}

/// If the event is of type T, return a pointer to it; otherwise nullptr.
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

