#ifndef _VA_EVENTUPSTREAM_HPP_
#define _VA_EVENTUPSTREAM_HPP_

#include "VaEntity.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace va
{

/// for event dispatching
class VaEventUpstream
{
protected:
    std::mutex              mtx;
    std::condition_variable cv;
    std::atomic< bool >     running{ false };
    std::thread             eventThread;

    /*
    Establish two indexes for registrants and events:
    One using event indexing for efficient event distribution
    One using entity indexing for efficient entity deregistration
    */
    // Store listeners indexed by event ID
    std::vector< std::vector< std::weak_ptr< VaEntity > > > Listeners;
    // For quick lookup of events registered by specific entities
    std::unordered_map< std::shared_ptr< VaEntity >, std::vector< size_t > > Listeners2;

public:
    // Register an entity to listen for a specific event
    void Register( size_t event_id, std::shared_ptr< VaEntity > entity );

    // Dispatch one event from the buffer
    virtual void DispatchOnce() = 0;

    // Event loop running in a separate thread
    virtual void thr_DispatchLoop() = 0;

    /*
    Three different unregistration methods:
    Unregister an entity and all its registered events
    Unregister all entities from a specific event
    Unregister a specific event from a specific entity
    */
    void UnRegister( std::shared_ptr< VaEntity > entity );
    void UnRegister( size_t event_id );
    void UnRegister( std::shared_ptr< VaEntity > entity, size_t event_id );

    // Start/stop the dispatch loop thread
    void Start();
    void Stop();

    // Check if the event loop is running
    bool IsRunning() const
    {
        return running;
    }

    VaEventUpstream()  = default;
    ~VaEventUpstream() = default;
};

}  // namespace va

#endif  // _VA_EVENTUPSTREAM_HPP_
