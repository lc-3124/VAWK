/*
 * VaEvent.hpp
 * Provides basic methods for event system
 *
 * e.g.
 * ......
 * struct MouseEvent : public va::event::EventBase
 * {
 *     EVENT_BASE_METHOD
 *     int x, y;
 *     bool isClick;
 * };
 * DEFINE_EVENT(MouseEvent)
 * ......
 * MouseEvent evt{100, 200, true};
 * ......
 * void handleEvent(const va::event::EventBase& e) {
 *     if (va::event::is_event<MouseEvent>(e)) {
 *         const MouseEvent& mouse = static_cast<const MouseEvent&>(e);
 *         std::cout << mouse.x << ", " << mouse.y;
 *     }
 * }
 * ......
 */

#ifndef _VAEVENT_HPP_
#define _VAEVENT_HPP_

#include <atomic>   // muti-threads security
#include <cstddef>  // size_t

namespace va
{
namespace event
{

    // event Base
    struct EventBase
    {
        virtual ~EventBase()      = default;
        virtual size_t id() const = 0;  // nesessary for subclass
    };

    // id maker
    inline size_t next_event_type_id()
    {
        static std::atomic< size_t > counter{ 0 };
        return counter++;  // Each call to auto increment returns a unique
                           // value
    }

    // get id of a type
    template < typename T > size_t event_type_id()
    {
        static_assert( std::is_base_of_v< va::event::EventBase, T >,
                       "T must derive from EventBase!" );
        static size_t type_id =
            next_event_type_id();  // Static variables ensure that the same
                                   // type is initialized only once
        return type_id;
    }

// for user , define an event
#define DEFINE_EVENT( T )                                              \
    template <> size_t va::event::event_type_id< T >()                 \
    {                                                                  \
        static_assert( std::is_base_of_v< va::event::EventBase, T > ); \
        static size_t type_id = va::event::next_event_type_id();       \
        return type_id;                                                \
    }

// subclass of EventBase sould include this macro
#define EVENT_BASE_METHOD                          \
    static size_t id()                             \
    {                                              \
        return va::event::event_type_id<           \
            std::decay_t< decltype( *this ) > >(); \
    }                                              \
    size_t id() const override { return id(); }

    template < typename T > constexpr bool is_event( const EventBase& e )
    {
        return e.id() == event_type_id< T >();
    }

    template < typename T > const T* getIf( const EventBase& e )
    {
        if ( is_event< T >( e ) ) return &static_cast< const T& >( e );
        return nullptr;
    }

}  // namespace event
}  // namespace va

#endif  // _VAEVENT_HPP_
