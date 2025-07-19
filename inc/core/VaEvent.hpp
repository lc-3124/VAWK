/*
 * VaEvent.hpp
 *
 * This header provides a set of utilities and macros for defining and working with custom event
 * types in a type-safe and extensible way. It is designed for use in event-driven systems, such as
 * GUI frameworks or game engines.
 *
 * Features:
 * - Unique type IDs for each event type at runtime.
 * - Macros to simplify event type definition.
 * - Type-safe utilities for event type checking and casting.
 *
 * Example usage:
 *
 * VA_EVENT_DEFINE(MouseEvent)
 *     int x, y;
 *     bool isClick;
 * VA_EVENT_DEFINE_END
 *
 * MouseEvent oneevent{100, 100, 1};
 *
 * void handleEvent(const EventBase& e)
 * {
 *     if (is_event<MouseEvent>(e))
 *     {
 *         auto& mouse = static_cast<const MouseEvent&>(e);
 *         std::cout << mouse.x << mouse.y;
 *     }
 * }
 *
 * Main components:
 * - event_type_id<T>(): Returns a unique ID for each event type.
 * - EVENT_BASE_METHOD(T): Implements the required id() method for the event.
 * - VA_EVENT_DEFINE / VA_EVENT_DEFINE_END: Macros to define new event types.
 * - is_event<T>(const EventBase&): Checks if an event is of type T.
 * - getIf<T>(const EventBase&): Returns a pointer to the event if it is of type T, otherwise
 * nullptr.
 *
 * All event types must inherit from va::event::EventBase (enforced by the macros).
 * Thread-safe unique ID generation is ensured via std::atomic.
 */

#ifndef _VAEVENT_HPP_
#define _VAEVENT_HPP_

#include <atomic>
#include <stdlib.h>

// return a unique id number
inline std::atomic< size_t >& global_id_counter()
{
    static std::atomic< size_t > counter{ 0 };
    return counter;
}

// get the type id of a type
template < typename T > size_t event_type_id()
{
    static size_t id = global_id_counter()++;
    return id;
}

// must be included in new event's definition.
#define EVENT_BASE_METHOD( T )       \
    size_t id() const override       \
    {                                \
        return event_type_id< T >(); \
    }

// but you can also use this to simply define a event
#define VA_EVENT_DEFINE( EventName )               \
    struct EventName : public va::event::EventBase \
    {                                              \
        EVENT_BASE_METHOD( EventName );

#define VA_EVENT_DEFINE_END \
    }                       \
    ;

namespace va
{
namespace event
{
    struct EventBase
    {
        virtual ~EventBase()      = default;
        virtual size_t id() const = 0;
    };

    template < typename T > constexpr bool is_event( const EventBase& e )
    {
        return e.id() == event_type_id< T >();
    }

    template < typename T > const T* getIf( const EventBase& e )
    {
        if ( is_event< T >( e ) )
            {
                return &static_cast< const T& >( e );
            }
        return nullptr;
    }
}
}
#endif
