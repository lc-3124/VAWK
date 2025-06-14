/*
 * VaEvent.hpp
 * provides a series of functions, used to define new Event type next
 *
 * e.g.
 * struct MouseEvent : public BaseEvent
 * {
 *  EVENT_BASE_METHOD;
 *      int x,y;
 *      bool isClick;
 * }
 * DEFINE_EVENT(MouseEvent);
 * ......
 * MouseEvent oneevent{100,100,1};
 * ......
 * void handleEvent(const EventBase& e)
 * {
 *      if(is_event<MouseEvent>(e))
 *      {
 *          auto& mouse = static_cast<const MouseEvent&>(e);
 *          std::cout<<mouse.x<<mouse.y;
 *          ......
 *      }
 * }
 * ......
 *
 */

#ifndef _VAEVENT_HPP_
#define _VAEVENT_HPP_

#include <stdlib.h>

// get the type id of a type
template < typename T > constexpr size_t event_type_id()
{
    return reinterpret_cast< size_t >( &event_type_id< T > );
}

struct EventBase
{
    virtual ~EventBase() = default;
};

// define event , for user ;
// it define a new struct from EventBase
#define DEFINE_EVENT( T )                                         \
    template <> size_t event_type_id< T >()                       \
    {                                                             \
        static_assert( std::is_base_of_v< EventBase, T > );       \
        return reinterpret_cast< size_t >( &event_type_id< T > ); \
    }

#define EVENT_BASE_METHOD                                     \
    static size_t id()                                        \
    {                                                         \
        static size_t id = reinterpret_cast< size_t >( &id ); \
        return id;                                            \
    }

template < typename T > constexpr bool is_event( const EventBase& e )
{
    return static_cast< const T& >( e ).id() == event_type_id< T >();
}

template < typename T > const T* getIf( const EventBase& e )
{
    if ( is_event< T >( e ) )
        {
            return &static_cast< const T& >( e );
        }
    return nullptr;
}

#endif
