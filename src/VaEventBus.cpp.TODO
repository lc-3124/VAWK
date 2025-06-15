#ifndef _VAEVENTBUS_CPP_
#define _VAEVENTBUS_CPP_

#include "core/VaEventBus.hpp"

using namespace va;
void VaEventBus::eventPush( event::EventBase* event )
{
    // remember not to course mem lea
    this->EventBuffer.push( event );
}
void VaEventBus::listenerRegister( event::EventBase* event, VaEnity* enity )
{
    listeners[ event->id() ].push_back( enity );
}
void VaEventBus::removeListener( VaEnity* enity )
{
    for ( auto it = this->listeners.begin(); it != listeners.end(); )
        {
            auto& vec = it->second;
            // Remove the value if it exists
            vec.erase( std::remove( vec.begin(), vec.end(), enity ), vec.end() );
            // If the vector is now empty, erase the entire key
            if ( vec.empty() )
                it = this->listeners.erase( it );  // Returns next valid iterator
            else
                ++it;
        }
}
void VaEventBus::eventDispatch()
{
    for ( auto iter_map : this->listeners )
        {
            for ( auto iter_vec : iter_map.second )
                {
                    iter_map->;
                }
        }
}

#endif
