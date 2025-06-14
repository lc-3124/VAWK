#ifndef _VAEVENTBUS_HPP_
#define _VAEVENTBUS_HPP_

#include "VaEvent.hpp"
#include "VaEnity.hpp"
#include <unordered_map>
#include <queue>

namespace va
{
/// 
class VaEventBus
{
protected:
    std::unordered_map<size_t,VaEnity*> listeners;
    std::queue<event::EventBase*> EventBuffer;
public:
    void eventPush(event::EventBase &event);
    void listenerRegister(VaEnity &enity);
    void eventDispatch();
};
extern VaEventBus va_event_bus;
}
#endif
