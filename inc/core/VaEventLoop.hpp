// VaEventLoop.hpp
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
#include <thread>
#include <atomic>

namespace va
{

/// Register, absorb, and dispatch events
class VaEventLoop
{
protected:
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> running{false};
    std::thread eventThread;

    /*
    Establish two indexes for registrants and events:
    One using event indexing for efficient event distribution
    One using entity indexing for efficient entity deregistration
    */
    // Store listeners indexed by event ID
    std::vector<std::vector<std::weak_ptr<VaEntity>>> Listeners;
    // For quick lookup of events registered by specific entities
    std::unordered_map<std::shared_ptr<VaEntity>, std::vector<size_t>> Listeners2;

    // Buffer for events from various sources
    // Using smart pointers for memory safety
    std::queue<std::shared_ptr<event::EventBase>> EventBuffer;

public:
    // Push an event into the buffer
    bool Push(std::shared_ptr<event::EventBase> event);

    // Register an entity to listen for a specific event
    void Register(size_t event_id, std::shared_ptr<VaEntity> entity);

    // Dispatch one event from the buffer
    void DispatchOnce();
    // Dispatch all events in the buffer
    void DispatchAll();
    // Event loop running in a separate thread
    void thr_DispatchLoop();

    /*
    Three different unregistration methods:
    Unregister an entity and all its registered events
    Unregister all entities from a specific event
    Unregister a specific event from a specific entity
    */
    void UnRegister(std::shared_ptr<VaEntity> entity);
    vo// VaEventLoop.hpp
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
#include <thread>
#include <atomic>

namespace va
{

/// Register, absorb, and dispatch events
class VaEventLoop
{
protected:
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> running{false};
    std::thread eventThread;

    /*
    Establish two indexes for registrants and events:
    One using event indexing for efficient event distribution
    One using entity indexing for efficient entity deregistration
    */
    // Store listeners indexed by event ID
    std::vector<std::vector<std::weak_ptr<VaEntity>>> Listeners;
    // For quick lookup of events registered by specific entities
    std::unordered_map<std::shared_ptr<VaEntity>, std::vector<size_t>> Listeners2;

    // Buffer for events from various sources
    // Using smart pointers for memory safety
    std::queue<std::shared_ptr<event::EventBase>> EventBuffer;

public:
    // Push an event into the buffer
    bool Push(std::shared_ptr<event::EventBase> event);

    // Register an entity to listen for a specific event
    void Register(size_t event_id, std::shared_ptr<VaEntity> entity);

    // Dispatch one event from the buffer
    void DispatchOnce();
    
    // Event loop running in a separate thread
    void thr_DispatchLoop();

    /*
    Three different unregistration methods:
    Unregister an entity and all its registered events
    Unregister all entities from a specific event
    Unregister a specific event from a specific entity
    */
    void UnRegister(std::shared_ptr<VaEntity> entity);
    void UnRegister(size_t event_id);
    void UnRegister(std::shared_ptr<VaEntity> entity, size_t event_id);

    // Start/stop the dispatch loop thread
    void Start();
    void Stop();

    // Check if the event loop is running
    bool IsRunning() const { return running; }

    // Singleton instance access
    static VaEventLoop& GetInstance();

private:
    // Private constructor for singleton
    VaEventLoop() = default;

    // Prevent copy and assignment
    VaEventLoop(const VaEventLoop&) = delete;
    VaEventLoop& operator=(const VaEventLoop&) = delete;

    // Destructor
    ~VaEventLoop() { Stop(); }
};

}  // namespace va

#endifid UnRegister(size_t event_id);
    void UnRegister(std::shared_ptr<VaEntity> entity, size_t event_id);

    // Start/stop the dispatch loop thread
    void Start();
    void Stop();

    // Check if the event loop is running
    bool IsRunning() const { return running; }

    // Singleton instance access
    static VaEventLoop& GetInstance();

private:
    // Private constructor for singleton
    VaEventLoop() = default;

    // Prevent copy and assignment
    VaEventLoop(const VaEventLoop&) = delete;
    VaEventLoop& operator=(const VaEventLoop&) = delete;

    // Destructor
    ~VaEventLoop() { Stop(); }
};

}  // namespace va

#endif
