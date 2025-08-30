// VaEventUpstream
#include "core/VaEventUpstream.hpp"
#include "core/entity_Wptr.hpp"
#include <algorithm>
#include <thread>

namespace va
{

void VaEventUpstream::Register( size_t event_id, entity_Sptr entity )
{
    if ( !entity )
        return;

    std::lock_guard< std::mutex > lock( mtx );

    // Resize listener vector if necessary
    if ( event_id >= Listeners.size() )
    {
        Listeners.resize( event_id + 10 );
    }

    // Avoid re-registration
    auto& vec = Listeners[ event_id ];
    auto  found =
      std::find_if( vec.begin(), vec.end(), [ &entity ]( const entity_Wptr wptr ) {
          auto sptr = wptr.lock();
          return sptr && sptr == entity;
      } );
    // not found
    if ( found == vec.end() )
    {
        vec.push_back( entity );
        Listeners2[ entity ].push_back( event_id );
    }
}

void VaEventUpstream::UnRegister( entity_Sptr entity )
{
    if ( !entity )
        return;

    std::lock_guard< std::mutex > lock( mtx );
    auto                          it = Listeners2.find( entity );

    if ( it != Listeners2.end() )
    {
        for ( size_t idx : it->second )
        {
            if ( idx < Listeners.size() )
            {
                auto& vec = Listeners[ idx ];
                vec.erase( std::remove_if( vec.begin(), vec.end(),
                                           [ &entity ]( const entity_Wptr& wptr ) {
                                               auto sptr = wptr.lock();
                                               return sptr && sptr == entity;
                                           } ),
                           vec.end() );
            }
        }
        Listeners2.erase( it );
    }
}

void VaEventUpstream::UnRegister( size_t event_id )
{
    std::lock_guard< std::mutex > lock( mtx );

    if ( event_id < Listeners.size() )
    {
        auto& vec = Listeners[ event_id ];

        // Update Listeners2 for all entities in this event
        for ( auto& wptr : vec )
        {
            auto sptr = wptr.lock();
            if ( sptr )
            {
                auto it = Listeners2.find( sptr );
                if ( it != Listeners2.end() )
                {
                    auto& idxs = it->second;
                    idxs.erase( std::remove( idxs.begin(), idxs.end(), event_id ), idxs.end() );

                    if ( idxs.empty() )
                    {
                        Listeners2.erase( it );
                    }
                }
            }
        }

        vec.clear();
    }
}

void VaEventUpstream::UnRegister( entity_Sptr entity, size_t event_id )
{
    if ( !entity || event_id >= Listeners.size() )
        return;

    std::lock_guard< std::mutex > lock( mtx );

    // Remove from Listeners
    auto& vec = Listeners[ event_id ];
    vec.erase( std::remove_if( vec.begin(), vec.end(),
                               [ &entity ]( const entity_Wptr& wptr ) {
                                   auto sptr = wptr.lock();
                                   return sptr && sptr == entity;
                               } ),
               vec.end() );

    // Update Listeners2
    auto it = Listeners2.find( entity );
    if ( it != Listeners2.end() )
    {
        auto& idxs = it->second;
        idxs.erase( std::remove( idxs.begin(), idxs.end(), event_id ), idxs.end() );

        if ( idxs.empty() )
        {
            Listeners2.erase( it );
        }
    }
}

void VaEventUpstream::eventloopStart()
{
    if ( running )
        return;
    running     = true;
    eventThread = std::thread( &VaEventUpstream::thr_DispatchLoop, this );
}

void VaEventUpstream::eventloopStop()
{
    if ( !running )
        return;

    running = false;
    cv.notify_all();  // Wake up the waiting thread

    if ( eventThread.joinable() )
    {
        eventThread.join();
    }
}

}  // namespace va
