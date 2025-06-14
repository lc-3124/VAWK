#include "inc/core/VaEvent.hpp"
#include <iostream>
#include <string>
using std::cout;

struct ShitEvent : public EventBase
{
    EVENT_BASE_METHOD;
    int         x;
    int         y;
    int         size;
    std::string name;
};
DEFINE_EVENT( ShitEvent )

void handleEvent( const EventBase& e )
{
    // if ( is_event< ShitEvent >( e ) )
    //   {
    auto& shit = static_cast< const ShitEvent& >( e );
    cout << shit.x << "\n";
    // }
}

int main()
{
    ShitEvent a_pile_of_shit;
    a_pile_of_shit.x    = 103;
    a_pile_of_shit.y    = 214;
    a_pile_of_shit.size = 100;
    a_pile_of_shit.name = "brother";

    EventBase& one      = a_pile_of_shit;
    handleEvent( one );
    return 0;
}
