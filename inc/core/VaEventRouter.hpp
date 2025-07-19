#ifndef _VA_EVENTROUTER_HPP_
#define _VA_EVENTROUTER_HPP_

#include "VaEventUpstream.hpp"

#include <memory>
#include <queue>

namespace va
{

/// Register, absorb, and dispatch events
class VaEventRouter : public VaEventUpstream
{
protected:
    // Buffer for events from various sources
    // Using smart pointers for memory safety
    std::queue< std::shared_ptr< event::EventBase > > EventBuffer;

public:
    // Push an event into the buffer
    bool Push( std::shared_ptr< event::EventBase > event );

    // Dispatch one event from the buffer
    void DispatchOnce() override;
    void thr_DispatchLoop() override;

    VaEventRouter() : VaEventUpstream() {};

    ~VaEventRouter()
    {
        eventloopStop();
    }
};

}  // namespace va

#endif
