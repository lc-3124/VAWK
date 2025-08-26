#include "core/VaEntity.hpp"
#include "core/VaEventUpstream.hpp"

namespace va
{

// Subsribe and Unsubscribe
void VaEntity::subscribe( std::shared_ptr< VaEventUpstream > upstream , std::vector< size_t > event_ids )
{
    if ( !upstream )
        return;
    for ( auto id : event_ids )
    {
        upstream->Register( id, shared_from_this() );
    }
}

void VaEntity::unsubscribe( std::shared_ptr< VaEventUpstream > upstream , std::vector< size_t > event_ids )
{
    if ( !upstream )
        return;
    for ( auto id : event_ids )
    {
        upstream->UnRegister( shared_from_this(), id );
    }
}

void VaEntity::unsubscribe( std::shared_ptr< VaEventUpstream > upstream )
{
    if ( !upstream )
        return;
    upstream->UnRegister( shared_from_this() );
}

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

void VaEntity::Raii()
{
    std::lock_guard< std::mutex > lock( this->upstream_entity_mtx );

    // Unsubscribe from all upstreams
    for ( auto& upstream : Upstreams )
    {
        if ( upstream )
        {
            upstream->UnRegister( shared_from_this() );
        }
    }
    Upstreams.clear();
    // Unregister all downEntitys
    for ( auto& entity : downEntitys )
    {
        if ( entity )
        {
            entity->Raii();
        }
    }
    downEntitys.clear();
}
};  // namespace va
