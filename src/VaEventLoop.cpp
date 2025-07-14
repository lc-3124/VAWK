// VaEventLoop.cpp
#include "VaEventLoop.hpp"
#include <algorithm>
#include <thread>

namespace va
{

bool VaEventLoop::Push(std::shared_ptr<event::EventBase> event)
{
    if (!event) return false;

    {
        std::lock_guard<std::mutex> lock(mtx);
        EventBuffer.push(event);
    }
    // Notify waiting thread about new event
    cv.notify_one();
    return true;
}

void VaEventLoop::Register(size_t event_id, std::shared_ptr<VaEntity> entity)
{
    if (!entity) return;

    std::lock_guard<std::mutex> lock(mtx);

    // Resize listener vector if necessary
    if (event_id >= Listeners.size())
    {
        Listeners.resize(event_id + 10);
    }

    // Avoid re-registration
    auto& vec = Listeners[event_id];
    auto found = std::find_if(vec.begin(), vec.end(),
        [&entity](const std::weak_ptr<VaEntity>& wptr) {
            auto sptr = wptr.lock();
            return sptr && sptr == entity;
        });
     // not found
    if (found == vec.end())
    {
        vec.push_back(entity);
        Listeners2[entity].push_back(event_id);
    }
}

void VaEventLoop::UnRegister(std::shared_ptr<VaEntity> entity)
{
    if (!entity) return;

    std::lock_guard<std::mutex> lock(mtx);
    auto it = Listeners2.find(entity);

    if (it != Listeners2.end())
    {
        for (size_t idx : it->second)
        {
            if (idx < Listeners.size())
            {
                auto& vec = Listeners[idx];
                vec.erase(
                    std::remove_if(vec.begin(), vec.end(),
                        [&entity](const std::weak_ptr<VaEntity>& wptr) {
                            auto sptr = wptr.lock();
                            return sptr && sptr == entity;
                        }),
                    vec.end()
                );
            }
        }
        Listeners2.erase(it);
    }
}

void VaEventLoop::UnRegister(size_t event_id)
{
    std::lock_guard<std::mutex> lock(mtx);

    if (event_id < Listeners.size())
    {
        auto& vec = Listeners[event_id];

        // Update Listeners2 for all entities in this event
        for (auto& wptr : vec)
        {
            auto sptr = wptr.lock();
            if (sptr)
            {
                auto it = Listeners2.find(sptr);
                if (it != Listeners2.end())
                {
                    auto& idxs = it->second;
                    idxs.erase(std::remove(idxs.begin(), idxs.end(), event_id), idxs.end());

                    if (idxs.empty())
                    {
                        Listeners2.erase(it);
                    }
                }
            }
        }

        vec.clear();
    }
}

void VaEventLoop::UnRegister(std::shared_ptr<VaEntity> entity, size_t event_id)
{
    if (!entity || event_id >= Listeners.size()) return;

    std::lock_guard<std::mutex> lock(mtx);

    // Remove from Listeners
    auto& vec = Listeners[event_id];
    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
            [&entity](const std::weak_ptr<VaEntity>& wptr) {
                auto sptr = wptr.lock();
                return sptr && sptr == entity;
            }),
        vec.end()
    );

    // Update Listeners2
    auto it = Listeners2.find(entity);
    if (it != Listeners2.end())
    {
        auto& idxs = it->second;
        idxs.erase(std::remove(idxs.begin(), idxs.end(), event_id), idxs.end());

        if (idxs.empty())
        {
            Listeners2.erase(it);
        }
    }
}

void VaEventLoop::DispatchOnce()
{
    std::shared_ptr<event::EventBase> event;
    size_t event_id = 0;
    bool has_event = false;
    // pop an event
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (EventBuffer.empty()) return;
        event = EventBuffer.front();
        event_id = event->id(); 
        EventBuffer.pop();
        has_event = true;
    }
    if (!has_event || !event) return;

    // check if there is a co vector , refer it
    std::vector<std::weak_ptr<VaEntity>>* listeners_ptr = nullptr;
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (event_id < Listeners.size() && !Listeners[event_id].empty()) {
            listeners_ptr = &Listeners[event_id];
        }
    }
    if (!listeners_ptr) return;

    // dispatch
    std::vector<std::weak_ptr<VaEntity>>& listeners = *listeners_ptr;
    for (auto it = listeners.begin(); it != listeners.end();) {
        auto sptr = it->lock();
        if (sptr) {
            sptr->eventPush(event);
            ++it;
        } else {
            // delete the ptr if its object has been removed
            it = listeners.erase(it);
        }
    }
}

void VaEventLoop::thr_DispatchLoop()
{
    while (running)
    {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait for event or termination signal
        cv.wait(lock, [this] {
            return !EventBuffer.empty() || !running;
        });

        // Check for termination
        if (!running) break;

        // Process one event using DispatchOnce
        DispatchOnce();
    }
}

void VaEventLoop::Start()
{
    if (running) return;

    running = true;
    eventThread = std::thread(&VaEventLoop::thr_DispatchLoop, this);
}

void VaEventLoop::Stop()
{
    if (!running) return;

    running = false;
    cv.notify_all(); // Wake up the waiting thread

    if (eventThread.joinable())
    {
        eventThread.join();
    }
}

}  // namespace va
