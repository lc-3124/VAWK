#include "../../inc/core/VaEvent.hpp"
#include <iostream>
#include <string>
using std::cout;
VA_EVENT_DEFINE( ShitEvent )
int         x;
int         y;
int         size;
std::string name;
VA_EVENT_DEFINE_END

VA_EVENT_DEFINE( ShiEvent )
int         x;
int         y;
int         size;
std::string name;
VA_EVENT_DEFINE_END

void handleEvent( const va::event::EventBase& e )
{
    if ( va::event::is_event< ShitEvent >( e ) )
        {
            auto& shit = static_cast< const ShitEvent& >( e );
            cout << "你在坐标 x:" << shit.x << " y:" << shit.y
                 << " 的位置拉了一坨屎,大小为:" << shit.size
                 << " 你给他取名叫做:" << shit.name << "\n";
        }
}

int main()
{
    ShitEvent a_pile_of_shit, ww;
    ShiEvent  a_pile_of_shi, wwi;
    a_pile_of_shit.x    = 103;
    a_pile_of_shit.y    = 214;
    a_pile_of_shit.size = 100;
    a_pile_of_shit.name = "brother";

    va::event::EventBase& one = a_pile_of_shit;
    handleEvent( one );
    cout << a_pile_of_shit.id() << "\n";
    cout << ww.id() << "\n";
    cout << wwi.id() << "\n";
    cout << a_pile_of_shi.id() << "\n";
    cout << event_type_id<ShiEvent>()<<"\n";
    return 0;
}
