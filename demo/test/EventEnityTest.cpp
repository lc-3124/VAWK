#include "../../inc/core/VaEventRouter.hpp"
#include <iostream>

va::VaEventRouter va_event_loop;

// Define a custom event type
VA_EVENT_DEFINE( MyEvent )
int value;
VA_EVENT_DEFINE_END

// Entity A: prints the event value
class EntityA : public va::VaEntity
{
protected:
    void handleEvent( std::shared_ptr< va::event::EventBase > event ) override
    {
        if ( va::event::is_event< MyEvent >( *event ) )
            {
                auto* myEvt = va::event::getIf< MyEvent >( *event );
                if ( myEvt )
                    {
                        std::cout << "[EntityA] Received MyEvent with value: " << myEvt->value
                                  << std::endl;
                    }
            }
    }
};

// Entity B: prints double the event value
class EntityB : public va::VaEntity
{
protected:
    void handleEvent( std::shared_ptr< va::event::EventBase > event ) override
    {
        if ( va::event::is_event< MyEvent >( *event ) )
            {
                auto* myEvt = va::event::getIf< MyEvent >( *event );
                if ( myEvt )
                    {
                        std::cout << "[EntityB] Received MyEvent, double value: "
                                  << ( myEvt->value * 2 ) << std::endl;
                    }
            }
    }
};

// Entity C: prints the event value squared
class EntityC : public va::VaEntity
{
protected:
    void handleEvent( std::shared_ptr< va::event::EventBase > event ) override
    {
        if ( va::event::is_event< MyEvent >( *event ) )
            {
                auto* myEvt = va::event::getIf< MyEvent >( *event );
                if ( myEvt )
                    {
                        std::cout << "[EntityC] Received MyEvent, squared value: "
                                  << ( myEvt->value * myEvt->value ) << std::endl;
                    }
            }
    }
};

int main()
{
    // Create entities
    auto a = std::make_shared< EntityA >();
    auto b = std::make_shared< EntityB >();
    auto c = std::make_shared< EntityC >();

    // Register entities to listen for MyEvent
    va_event_loop.Register( event_type_id< MyEvent >(), a );
    va_event_loop.Register( event_type_id< MyEvent >(), b );
    va_event_loop.Register( event_type_id< MyEvent >(), c );

    // Create and push an event
    auto evt   = std::make_shared< MyEvent >();
    evt->value = 42;
    va_event_loop.Push( evt );

    // Dispatch the event (each entity will handle it in its own way)
    // Create and push an event
    auto evt2   = std::make_shared< MyEvent >();
    evt2->value = 38;
    va_event_loop.Push( evt2 );

    // Dispatch the event (each entity will handle it in its own way)
    va_event_loop.DispatchOnce();
    va_event_loop.DispatchOnce();

    a->processOneEvent();
    b->processOneEvent();
    c->processOneEvent();
    a->processOneEvent();
    b->processOneEvent();
    c->processOneEvent();
    return 0;
}
