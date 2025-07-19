#ifndef _VA_INPUT_HPP
#define _VA_INPUT_HPP

#include "core/VaEventUpstream.hpp"

VA_EVENT_DEFINE(VA_MOUSE_LEFT_CLICK)
    float x,y;
    int duration = -1; // time_t i


namespace va
{
class VaInput
{
protected:

    

public:

    // Start or Stop mouse loop 
    void Start();
    void Stop();

};
}

#endif  //_VA_INPUT_HPP
