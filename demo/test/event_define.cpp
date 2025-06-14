#include "core/VaEvent.hpp"
#include <iostream>
#include <string>
using std::cout;
struct ShitEvent : public va::event::EventBase
{
    EVENT_BASE_METHOD;
    int         x;
    int         y;
    int         size;
    std::string name;
};
DEFINE_EVENT( ShitEvent )

void handleEvent( const va::event::EventBase& e )
{
    if ( va::event::is_event< ShitEvent >( e ) )
        {
            auto& shit = static_cast< const ShitEvent& >( e );
            cout << "你在坐标 x:" << shit.x << " y:" << shit.y
                 << " 的位置拉了一坨屎,大小为:" << shit.size << " 你给他取名叫做:" << shit.name
                 << "\n";
        }
}

int main()
{
    ShitEvent a_pile_of_shit,ww;
    a_pile_of_shit.x    = 103;
    a_pile_of_shit.y    = 214;
    a_pile_of_shit.size = 100;
    a_pile_of_shit.name = "brother";

    va::event::EventBase& one = a_pile_of_shit;
    handleEvent( one );
    cout<<a_pile_of_shit.id()<<"\n";
    cout<<ww.id()<<"\n";
    return 0;
}
