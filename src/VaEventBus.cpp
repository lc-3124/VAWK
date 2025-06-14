#ifndef _VAEVENTBUS_CPP_
#define _VAEVENTBUS_CPP_

#include "core/VaEventBus.hpp"

using namespace va;
void VaEventBus::eventPush(event::EventBase &event)
{
    this->EventBuffer.push(&event);
}

#endif
