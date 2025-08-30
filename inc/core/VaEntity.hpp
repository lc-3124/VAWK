#ifndef _VA_ENTITY_HPP_
#define _VA_ENTITY_HPP_

#include "VaEvent.hpp"
#include "entity_Sptr.hpp"
#include "enable_entity_shared_from_this.hpp"
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace va
{

// Pre-declare VaEventUpstream
class VaEventUpstream;

/*
 * The base class for all event-receiving entities in the event system.
 *
 * VaEntity provides a thread-safe event buffer and a virtual interface for handling events.
 * Users should inherit from VaEntity and implement handleEvent() to define custom event logic.
 *
 * Typical usage:
 *
 * class MyEntity : public va::VaEntity {
 * protected:
 *     // Implement your own event handling logic here!
 *     void handleEvent(std::shared_ptr<event::EventBase> event) override {
 *         if (va::event::is_event<MyEvent>(*event)) {
 *             auto* myEvt = va::event::getIf<MyEvent>(*event);
 *             if (myEvt) {
 *                 // Do something with myEvt
 *             }
 *         }
 *     }
 * };
 *
 * MyEntity entity;
 * entity.eventPush(myEventPtr); // Pushes and immediately handles the event
 * entity.processOneEvent();     // Optionally, process one event from the buffer
 */
class VaEntity :  public enable_entity_shared_from_this
{
  protected:
    // TODO need a lock-free queue
    std::mutex mtx , upstream_entity_mtx;  ///< Mutex for thread-safe access to the event buffer

    // Store all events received by this entity (FIFO queue)
    std::queue< std::shared_ptr< event::EventBase > > EventBuffer;

    // Store indexes of upstreams and downEntitys, they are used to mark
    // where the Entity subscribes events from and what sub-Entities it contains.
    std::vector< std::shared_ptr< VaEventUpstream > > Upstreams;
    std::vector< entity_Sptr > downEntitys;
    
    /*
     * Handle a single event.
     * Users should override this method in derived classes to implement custom event handling
     * logic. This function is called by eventPush() and processOneEvent().
     *
     * event: The event to handle.
     */
    virtual void handleEvent( std::shared_ptr< event::EventBase > event ) = 0;

    /*
     * String label , used to check it while running
     */
    std::string label;

    /**/

  public:

    /*
     * Safely subscribe to an upstream event source.
     * This function registers the entity with the specified VaEventUpstream to receive events.
     * You must use this mothod to subscribe to events instead of using one in `VaUpstream`
     */
    void subscribe( std::shared_ptr< VaEventUpstream > upstream , std::vector< size_t > event_ids );
    /*
     * Safely unsubscribe from an upstream event source.
     * This function unregisters the entity from the specified VaEventUpstream, stopping it from
     * receiving further events.
     */
    void unsubscribe( std::shared_ptr< VaEventUpstream > upstream , std::vector< size_t > event_ids );
    void unsubscribe( std::shared_ptr< VaEventUpstream > upstream );
    
    /*
     * Push an event into the buffer and 'handlevent' method will handle it.
     * This function is thread-safe. The event is pushed into the buffer and handleEvent() is
     * called.
     *
     * event: The event to push.
     */
    virtual void eventPush( std::shared_ptr< event::EventBase > event );

    /*
     * Process one event from the buffer (FIFO).
     * If the buffer is not empty, pops and handles the front event.
     * Useful for asynchronous or deferred event processing.
     *  
     *  return:
     *      1: sucess  ( callback-function has been finished )
     *      0: bad event
     *     -1: empty buffer
     */
    virtual int processOneEvent();

    /* 
     * List of Data Interface
     */
    std::string getLabel();
    void        setLabel( std::string nameLabel ){this->label = nameLabel;};

    /*
     * Destructor, used to clean up the event registration and 
     * clean other resources.
     */
    void Raii();
    virtual ~VaEntity()
    {
      Raii();
    };
};

};  // namespace va

#endif
