#include "core/VaEventLoop.hpp"
#include <algorithm>

namespace va
{

bool VaEventLoop::Push( std::shared_ptr< event::EventBase > event )
{
    std::lock_guard< std::mutex > lock( mtx );
    if ( event.get() == nullptr ) return 0;
    EventBuffer.push( event );
    return 1;
}

void VaEventLoop::Register( size_t index, VaEntity* entity )
{
    std::lock_guard< std::mutex > lock( mtx );
    if ( index >= Listeners.size() )
        {
            Listeners.resize( index + 1 );
        }

    // avoid re-registering
    auto& vec = Listeners[ index ];
    if ( std::find( vec.begin(), vec.end(), entity ) == vec.end() )
        {
            vec.push_back( entity );
            Listeners2[ entity ].push_back( index );
        }
}

void VaEventLoop::UnRegister( VaEntity* entity )
{
    std::lock_guard< std::mutex > lock( mtx );
    auto                          it = Listeners2.find( entity );
    if ( it != Listeners2.end() )
        {
            // remove from Listeners
            for ( size_t idx : it->second )
                {
                    if ( idx < Listeners.size() )
                        {
                            auto& vec = Listeners[ idx ];
                            vec.erase( std::remove( vec.begin(), vec.end(),
                                                    entity ),
                                       vec.end() );
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
            // Remove all event_ids corresponding to entities from
            // Listeners2 reverse mapping
            for ( VaEntity* entity : Listeners[ event_id ] )
                {
                    auto it = Listeners2.find( entity );
                    if ( it != Listeners2.end() )
                        {
                            auto& idxs = it->second;
                            idxs.erase( std::remove( idxs.begin(),
                                                     idxs.end(),
                                                     event_id ),
                                        idxs.end() );
                            if ( idxs.empty() ) Listeners2.erase( it );
                        }
                }
            Listeners[ event_id ].clear();
        }
}

void VaEventLoop::UnRegister( VaEntity* entity, size_t event_id )
{
    std::lock_guard< std::mutex > lock( mtx );
    // Listeners[event_id] , remove entity
    if ( event_id < Listeners.size() )
        {
            auto& vec = Listeners[ event_id ];
            vec.erase( std::remove( vec.begin(), vec.end(), entity ),
                       vec.end() );
        }
    // Listeners2[entity] , remove event_id
    auto it = Listeners2.find( entity );
    if ( it != Listeners2.end() )
        {
            auto& idxs = it->second;
            idxs.erase( std::remove( idxs.begin(), idxs.end(), event_id ),
                        idxs.end() );
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
    // it looks like the event is not going to be overflowed 
    // because in the Push function , we have resized the EventBuffer
    // However, I also want to to check the event id
    size_t eid = oneEvent->id();
    if (eid >= Listeners.size()) return;
    // push event to all listeners 
    // this action is without lock , because I need it to be fast
    for (auto iter : Listeners[eid])
    {
        iter->eventPush(oneEvent);
    }
}

}  // namespace va
