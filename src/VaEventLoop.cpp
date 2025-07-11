#include "core/VaEventLoop.hpp"
#include <algorithm>
#include <thread>

namespace va
{

bool VaEventLoop::Push( std::shared_ptr< event::EventBase > event )
{
    std::lock_guard< std::mutex > lock( mtx );
    if ( event.get() == nullptr ) return 0;
    EventBuffer.push( event );
    return 1;
}

void VaEventLoop::Register( size_t index, std::shared_ptr<VaEntity> entity )
{
    std::lock_guard< std::mutex > lock( mtx );
    if ( index >= Listeners.size() )
    {
        Listeners.resize( index + 1 );
    }

    // avoid re-registering
    auto& vec = Listeners[ index ];
    auto found = std::find_if(vec.begin(), vec.end(),
        [&entity](const std::weak_ptr<VaEntity>& wptr) {
            auto sptr = wptr.lock();
            return sptr && sptr == entity;
        });
    if ( found == vec.end() )
    {
        vec.push_back( entity );
        Listeners2[ entity ].push_back( index );
    }
}

void VaEventLoop::UnRegister( std::shared_ptr<VaEntity> entity )
{
    std::lock_guard< std::mutex > lock( mtx );
    auto it = Listeners2.find( entity );
    if ( it != Listeners2.end() )
    {
        for ( size_t idx : it->second )
        {
            if ( idx < Listeners.size() )
            {
                auto& vec = Listeners[ idx ];
                vec.erase(
                    std::remove_if(vec.begin(), vec.end(),
                        [&entity](const std::weak_ptr<VaEntity>& wptr) {
                            auto sptr = wptr.lock();
                            return sptr && sptr == entity;
                        }),
                    vec.end()
                );
            }
        }
        Listeners2.erase( it );
    }
}

void VaEventLoop::UnRegister( size_t event_id )
{
    std::lock_guard< std::mutex > lock( mtx );
    if ( event_id < Listeners.size() )
    {
        for ( auto& wptr : Listeners[ event_id ] )
        {
            auto sptr = wptr.lock();
            if (sptr)
            {
                auto it = Listeners2.find( sptr );
                if ( it != Listeners2.end() )
                {
                    auto& idxs = it->second;
                    idxs.erase( std::remove( idxs.begin(), idxs.end(), event_id ), idxs.end() );
                    if ( idxs.empty() ) Listeners2.erase( it );
                }
            }
        }
        Listeners[ event_id ].clear();
    }
}

void VaEventLoop::UnRegister( std::shared_ptr<VaEntity> entity, size_t event_id )
{
    std::lock_guard< std::mutex > lock( mtx );
    if ( event_id < Listeners.size() )
    {
        auto& vec = Listeners[ event_id ];
        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [&entity](const std::weak_ptr<VaEntity>& wptr) {
                    auto sptr = wptr.lock();
                    return sptr && sptr == entity;
                }),
            vec.end()
        );
    }
    auto it = Listeners2.find( entity );
    if ( it != Listeners2.end() )
    {
        auto& idxs = it->second;
        idxs.erase( std::remove( idxs.begin(), idxs.end(), event_id ), idxs.end() );
        if ( idxs.empty() ) Listeners2.erase( it );
    }
}

void VaEventLoop::DispatchOnce()
{
    std::shared_ptr<event::EventBase> oneEvent;
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (EventBuffer.empty()) return;
        oneEvent = EventBuffer.front();
        EventBuffer.pop();
    }
    size_t eid = oneEvent->id();
    if (eid >= Listeners.size()) return;
    for (auto& wptr : Listeners[eid])
    {
        auto sptr = wptr.lock();
        if (sptr)
        {
            sptr->eventPush(oneEvent);
        }
    }
}

void VaEventLoop::DispatchAll()
{
    std::lock_guard<std::mutex> lock(mtx);
    while (!EventBuffer.empty())
    {
        auto oneEvent = EventBuffer.front();
        EventBuffer.pop();
        size_t eid = oneEvent->id();
        if (eid >= Listeners.size()) continue;
        for (auto& wptr : Listeners[eid])
        {
            auto sptr = wptr.lock();
            if (sptr)
            {
                sptr->eventPush(oneEvent);
            }
        }
    }
}

// this function is used to run the event loop in a separate thread
void VaEventLoop::thr_DispatchLoop()
{
    while (true)
    {
        DispatchOnce();
        // sleep for a while to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

}  // namespace va
