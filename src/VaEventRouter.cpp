// VaEventRouter.cpp
#include "core/VaEventRouter.hpp"

namespace va
{

bool VaEventRouter::Push( std::shared_ptr< event::EventBase > event )
{
    if ( !event ) return false;

    {
        std::lock_guard< std::mutex > lock( mtx );
        EventBuffer.push( event );
    }
    // Notify waiting thread about new event
    cv.notify_one();
    return true;
}
void VaEventRouter::DispatchOnce()
{
    std::shared_ptr< event::EventBase > event;
    size_t                              event_id  = 0;
    bool                                has_event = false;
    // pop an event
    {
        std::lock_guard< std::mutex > lock( mtx );
        if ( EventBuffer.empty() )
            return;
        event    = EventBuffer.front();
        event_id = event->id();
        EventBuffer.pop();
        has_event = true;
    }
    if ( !has_event || !event )
        return;

    // check if there is a co vector , refer it
    std::vector< std::weak_ptr< VaEntity > >* listeners_ptr = nullptr;
    {
        std::lock_guard< std::mutex > lock( mtx );
        if ( event_id < Listeners.size() && !Listeners[ event_id ].empty() )
        {
            listeners_ptr = &Listeners[ event_id ];
        }
    }
    if ( !listeners_ptr )
        return;

    // dispatch
    std::vector< std::weak_ptr< VaEntity > >& listeners = *listeners_ptr;
    for ( auto it = listeners.begin(); it != listeners.end(); )
    {
        auto sptr = it->lock();
        if ( sptr )
        {
            sptr->eventPush( event );
            ++it;
        }
        else
        {
            // delete the ptr if its object has been removed
            it = listeners.erase( it );
        }
    }
}

void VaEventRouter::thr_DispatchLoop() {
    std::unique_lock<std::mutex> lk(cv_mtx);
    while (running) {
        cv.wait(lk, [this] { 
        return !EventBuffer.empty() || !running.load(); 
         });
        if (!running) break;
        DispatchOnce();
    }

    while (!EventBuffer.empty()) {
        EventBuffer.pop();
    }
}
}  // namespace va
