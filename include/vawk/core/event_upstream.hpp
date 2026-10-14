#ifndef VAWK_EVENT_UPSTREAM_HPP
#define VAWK_EVENT_UPSTREAM_HPP

//- vawk::EventUpstream — 事件源基类
//
//  EventUpstream 泛指任何产生事件、并把事件投递给已注册 Entity
//  实例的组件。它维护两张索引：
//
//    listeners_[event_id]   → 需要该事件类型的 Entity* 列表
//    entity_events_[entity] → 该实体已注册的事件类型 ID 列表
//                             （用于批量注销）
//
//  实体以裸指针存储。调用方必须保证每个 Entity 在被销毁前已经注销。
//
//  子类必须实现 dispatch_once() 与 dispatch_loop()。基类提供
//  start_event_loop() / stop_event_loop()，用于在后台线程中运行
//  dispatch_loop()。

#include "entity.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace vawk {

/**
 * @brief 事件源基类：产生事件并投递给已注册的 Entity。
 * @note 实体以裸指针存储；调用方必须保证 Entity 在销毁前已注销。
 */
class EventUpstream {
  public:
    EventUpstream() = default;
    virtual ~EventUpstream();

    //-- 注册

    /**
     * @brief 注册一个实体，使其接收给定类型 ID 的事件。
     * @param event_id 事件类型 ID。
     * @param entity   监听者。
     */
    void register_listener(size_t event_id, Entity* entity);

    /**
     * @brief 从该实体已订阅的所有事件类型中注销它。
     * @param entity 待注销的监听者。
     */
    void unregister_listener(Entity* entity);

    /**
     * @brief 从某个事件类型上注销所有实体。
     * @param event_id 事件类型 ID。
     */
    void unregister_listener(size_t event_id);

    /**
     * @brief 从某个事件类型上注销指定实体。
     * @param entity   监听者。
     * @param event_id 事件类型 ID。
     */
    void unregister_listener(Entity* entity, size_t event_id);

    //-- 派发（纯虚）

    /** @brief 从事件源向其监听者派发恰好一个事件。 */
    virtual void dispatch_once() = 0;

    /**
     * @brief 后台事件循环：在紧凑循环中反复调用 dispatch_once()。
     * @note 由 start_event_loop() 在专用线程中使用。
     */
    virtual void dispatch_loop() = 0;

    //-- 生命周期

    /** @brief 在后台线程中启动事件循环。 */
    void start_event_loop();

    /** @brief 通知事件循环停止，并 join 该线程。 */
    void stop_event_loop();

    /** @brief 检查事件循环线程当前是否正在运行。 */
    bool is_running() const { return running_; }

  protected:
    std::mutex              mutex_;
    std::mutex              cv_mutex_;
    std::condition_variable cv_;
    std::atomic<bool>       running_{false};
    std::thread             event_thread_;

    // listeners_[event_id] = 实体列表
    std::vector<std::vector<Entity*>> listeners_;

    // entity_events_[entity] = 事件类型 ID 列表
    std::unordered_map<Entity*, std::vector<size_t>> entity_events_;
};

}  // namespace vawk

#endif  // VAWK_EVENT_UPSTREAM_HPP
