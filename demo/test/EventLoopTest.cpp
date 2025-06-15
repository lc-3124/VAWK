#include "../../inc/core/VaEventLoop.hpp"

VA_EVENT_DEFINE( ShitEvent )
int x, y;
VA_EVENT_DEFINE_END
VA_EVENT_DEFINE( ShiEvent )
int x, y;
VA_EVENT_DEFINE_END
VA_EVENT_DEFINE( ShEvent )
int x, y;
VA_EVENT_DEFINE_END
VA_EVENT_DEFINE( SEvent )
int x, y;
VA_EVENT_DEFINE_END

int main()
{
    std::shared_ptr< ShitEvent > shit( new ShitEvent );
    std::shared_ptr< ShiEvent >  shi( new ShiEvent );
    std::shared_ptr< ShEvent >   sh( new ShEvent );
    std::shared_ptr< SEvent >    s( new SEvent );
    shit.get()->x = 102;
    shit.get()->y = 103;
    auto a        = new va::VaEntity;
    va::va_event_loop.Register( shit.get()->id(), a );
    va::va_event_loop.Register( shi.get()->id(), a );
    va::va_event_loop.Register( sh.get()->id(), a );
    va::va_event_loop.Register( event_type_id< SEvent >(), a );
    va::va_event_loop.Push( shit );
    va::va_event_loop.Push( shi );
    va::va_event_loop.Push( sh );
    va::va_event_loop.Push( s );
    va::va_event_loop.Push( shit );
    va::va_event_loop.UnRegister(shi.get()->id());
    va::va_event_loop.DispatchOnce();
    va::va_event_loop.DispatchOnce();
    va::va_event_loop.DispatchOnce();
    va::va_event_loop.DispatchOnce();
    va::va_event_loop.DispatchOnce();
    va::va_event_loop.UnRegister(a);
    va::va_event_loop.Push( shi );
    va::va_event_loop.DispatchOnce();
    
    //output 0230 if it is correctlly worked 

    // PS : OK

    // changed code of VaEntity , this file had been invalid
    return 0;
}
