#ifndef _VA_EVENTLOOP_CPP
#define _VA_EVENTLOOP_CPP

#include "core/VaEventLoop.hpp"
#include <algorithm>

//TODO:
// TO FIX :
// DispatchOnce() 未检查 EventBuffer 是否为空，直接 front() 和 pop() 可能导致程序崩溃（未捕获的异常或未定义行为）。
// Push() 方法未对 event 参数做空指针检查，理论上可以插入空事件。
// TO ADD:
// 没想好
namespace va
{

void VaEventLoop::Push( std::shared_ptr< event::EventBase > event )
{
    std::lock_guard< std::mutex > lock( mtx );
    EventBuffer.push( event );
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
    std::lock_guard< std::mutex > lock( mtx );
    auto                          oneEvent = this->EventBuffer.front();
    this->EventBuffer.pop();

    for ( auto iter : this->Listeners[oneEvent.get()->id()] )
        {
            iter->eventPush( oneEvent );
        }
}

}  // namespace va

#endif
