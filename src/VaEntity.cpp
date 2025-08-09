#include "core/VaEntity.hpp"

namespace va
{

// Push an event into the entity's buffer and handle it
void VaEntity::eventPush( std::shared_ptr< event::EventBase > event )
{
    std::lock_guard< std::mutex > lock( mtx );
    this->EventBuffer.push( event );
}

int VaEntity::processOneEvent()
{
    auto one_event = std::shared_ptr< event::EventBase >( nullptr );
    {
        std::lock_guard< std::mutex > lock( mtx );
        if( this->EventBuffer.empty() )return -1;
        one_event = this->EventBuffer.front();
        this->EventBuffer.pop();
        if ( one_event.get() == nullptr )
            return 0;
    }
    // handleEvent is a virtual function , need usr to implement it
    this->handleEvent( one_event );
    return 1;
}

}  // namespace va
