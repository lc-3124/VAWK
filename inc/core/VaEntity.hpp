#ifndef _VA_ENTITY_HPP_
#define _VA_ENTITY_HPP_

#include "VaEvent.hpp"
#include <memory>
#include <mutex>
#include <queue>

namespace va
{

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
class VaEntity
{
protected:
// TODO need a lock-free queue
    std::mutex mtx; ///< Mutex for thread-safe access to the event buffer

    // Store all events received by this entity (FIFO queue)
    std::queue< std::shared_ptr< event::EventBase > > EventBuffer;

    /*
     * Handle a single event.
     * Users should override this method in derived classes to implement custom event handling logic.
     * This function is called by eventPush() and processOneEvent().
     * 
     * event: The event to handle.
     */
    virtual void handleEvent(std::shared_ptr<event::EventBase> event) = 0;

public:
    /*
     * Push an event into the buffer and 'handlevent' method will handle it.
     * This function is thread-safe. The event is pushed into the buffer and handleEvent() is called.
     * 
     * event: The event to push.
     */
    virtual void eventPush(std::shared_ptr<event::EventBase> event);

    /*
     * Process one event from the buffer (FIFO).
     * If the buffer is not empty, pops and handles the front event.
     * Useful for asynchronous or deferred event processing.
     */
    virtual void processOneEvent();

    virtual ~VaEntity() = default;
};

} // namespace va

#endif
