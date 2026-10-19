#ifndef VAWK_EVENT_UPSTREAM_HPP
#define VAWK_EVENT_UPSTREAM_HPP

//- vawk::EventUpstream — 事件源基类
//
//  EventUpstream 泛指任何产生事件、并把事件投递给已注册 Entity 的组件。
//  它维护两张互为索引的表：
//
//    listeners_[event_id]        → vector<Listener{entity, sub_slot}>
//    entity_events_[entity.get()] → vector<Subscription{event_id, list_slot}>
//
//  两张表都记录对方槽位，注销时 swap-pop 并修正被移动者的槽位，O(1)。
//
//  订阅即持有：首次订阅时 upstream 会以 HolderKind::Router 的身份
//  acquire() 该实体。实体注销订阅时由 detach() 完成放手。
//
//  子类必须实现 dispatch_once() 与 dispatch_loop()；基类提供
//  start_event_loop() / stop_event_loop() 在后台线程跑 dispatch_loop()。

#include "vawk/core/entity.hpp"
#include "vawk/core/holder.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace vawk {

/**
 * @brief 事件源基类：维护订阅索引，并向监听者投递事件。
 * @note  监听者以 shared_ptr 持有，因此实体在订阅期间不会被销毁。
 */
class EventUpstream : public Holder {
  public:
    EventUpstream()          = default;
    EventUpstream(const EventUpstream&)            = delete;
    EventUpstream& operator=(const EventUpstream&) = delete;

    /** @brief 析构：先停事件循环，再由 Holder 析构注销全部持有记录。 */
    ~EventUpstream() override;

    //-- 订阅

    /**
     * @brief 订阅一个事件类型；同一实体重复订阅同一 ID 幂等。
     * @param event_id 事件类型 ID。
     * @param entity   监听者。
     * @note  首次订阅会 acquire() 该实体（Router 持有）。
     */
    void subscribe(std::size_t event_id, std::shared_ptr<Entity> entity);

    /**
     * @brief 注销某实体在某个事件类型上的订阅。
     * @param event_id 事件类型 ID。
     * @param entity   监听者。
     */
    void unsubscribe(std::size_t event_id, const Entity* entity);

    /** @brief 注销某实体的全部订阅。 */
    void unsubscribe_all(const Entity* entity);

    /**
     * @brief 实体释放时的统一出口：注销全部订阅并放开 Router 持有。
     * @param entity 待摘除的实体。
     */
    void detach(const Entity* entity);

    /**
     * @brief 实体请求放手：先摘掉订阅索引里的强引用，再放开持有记录。
     * @param entity 请求释放的实体。
     * @note  只丢记录不摘订阅会让 listeners_ 继续强引用实体，
     *        孤儿判定就永远无法真正回收实体。
     */
    void holder_release(std::shared_ptr<Entity> entity) override;

    //-- 派发（纯虚）

    /** @brief 从事件源向其监听者派发恰好一个事件。 */
    virtual void dispatch_once() = 0;

    /** @brief 后台事件循环；由 start_event_loop() 在专用线程中运行。 */
    virtual void dispatch_loop() = 0;

    //-- 生命周期

    /** @brief 在后台线程中启动事件循环。 */
    void start_event_loop();

    /** @brief 通知事件循环停止，并 join 该线程。 */
    void stop_event_loop();

    /** @brief 事件循环线程当前是否正在运行。 */
    bool is_running() const { return running_.load(); }

  protected:
    /** @brief 唤醒阻塞中的 dispatch_loop()；子类必须持自己的队列锁通知。 */
    virtual void wake_dispatch_thread() {}

    /**
     * @brief 锁内快照某个事件类型的监听者，供锁外投递。
     * @param event_id 事件类型 ID。
     * @return 监听者强引用列表；未注册时为空。
     */
    std::vector<std::shared_ptr<Entity>> snapshot_listeners(std::size_t event_id) const;

    std::atomic<bool> running_{false}; ///< 事件循环是否继续

  private:
    // listeners_ 的一项：实体 + 它在 entity_events_ 中的槽位。
    struct Listener {
        std::shared_ptr<Entity> entity;
        std::size_t             sub_slot = 0;
    };

    // entity_events_ 的一项：事件 ID + 它在 listeners_ 中的槽位。
    struct Subscription {
        std::size_t event_id  = 0;
        std::size_t list_slot = 0;
    };

    mutable std::mutex mtx_listeners_; ///< 保护两张索引表
    std::vector<std::vector<Listener>> listeners_; ///< 按事件 ID 的监听者
    std::unordered_map<const Entity*, std::vector<Subscription>> entity_events_; ///< 按实体的订阅

    std::mutex  mtx_lifecycle_; ///< 串行化 start / stop
    std::thread event_thread_; ///< 事件循环线程
};

}  // namespace vawk

#endif  // VAWK_EVENT_UPSTREAM_HPP
