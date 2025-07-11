#ifndef _VA_EVENTLOOP_HPP_
#define _VA_EVENTLOOP_HPP_

#include "VaEntity.hpp"
#include "VaEvent.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>
#include <condition_variable>

namespace va
{

/// register , absorb , and dispatch events
class VaEventLoop
{
    protected:
    std::mutex mtx;
    std::condition_variable cv;
    std::mutex mtx_cv;
    /*
     * Establish two indexes for registrants and events, one using event
     *indexing controls to facilitate event distribution, and the other
     *using control indexing events to facilitate control
     *deregistration.
     */
    // Store events and controls registered here
    std::vector< std::vector< std::weak_ptr<VaEntity> > > Listeners;
    // for deleteing ,
    std::unordered_map< std::shared_ptr<VaEntity>, std::vector< size_t > > Listeners2;

    // Cache global events pushed from various sources
    // Use smart pointers to protect memory.
    std::queue< std::shared_ptr< event::EventBase > > EventBuffer;

    public:
    // push a event (struct with argments)
    bool Push( std::shared_ptr< event::EventBase > event );

    // push entity's ptr into this->Listeners ,by index
    void Register( size_t event_id, std::shared_ptr<VaEntity> entity );

    // hand out events to all the registered entity
    void DispatchOnce();
    void DispatchAll();
    void thr_DispatchLoop();

    /*
     * Three different deletion methods
     * 1. Log an entity out of the eventloop and erase the entity and all
     * events registered with it.
     * 2. Cancel an event and erase all entity registrations for that
     * event.
     * 3. Only erase the corresponding event registered by the.
     * corresponding entity.
     * Of course, if an entity has not registered any
     * events or an event has not been registered by any entity, they
     * should not appear in Listeners2 or Listeners' index
     */
    void UnRegister( std::shared_ptr<VaEntity> entity );
    void UnRegister( size_t event );
    void UnRegister( std::shared_ptr<VaEntity> entity, size_t event_id );
};

static VaEventLoop va_event_loop;

};
#endif
