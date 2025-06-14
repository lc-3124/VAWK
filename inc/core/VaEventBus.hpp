#ifndef _VAEVENTBUS_HPP_
#define _VAEVENTBUS_HPP_

#include "VaEnity.hpp"
#include "VaEvent.hpp"
#include <queue>
#include <unordered_map>
#include <vector>

namespace va
{
///
class VaEventBus
{
protected:
    std::unordered_map< size_t, std::vector<VaEnity*> > listeners;
    std::queue< event::EventBase* >        EventBuffer;

public:
    void eventPush( event::EventBase* event );
    void listenerRegister( event::EventBase* event, VaEnity* enity );
    void removeListener( VaEnity* enity );
    void eventDispatch();
};
extern VaEventBus va_event_bus;
}
#endif
