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
//  两张索引表都只保存 weak_ptr（Listener::raw 仅用于比较 / 定位）：
//  订阅关系不延长实体生命，唯一强引用在 detail::EntityStore 里。
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
 * @note  监听者只以 weak_ptr 记录，订阅本身不延长实体生命；投递时
 *        lock() 失败（实体已释放）的监听者会被跳过。
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
     * @brief 某个事件类型当前的订阅者快照（供状态展示）。
     * @param event_id 事件类型 ID。
     * @return 仍存活的订阅者列表；无订阅或全部过期时为空。
     * @note   快照中的 shared_ptr 只在快照存活期间临时延长实体生命周期。
     */
    std::vector<std::shared_ptr<Entity>> subscribers(std::size_t event_id) const;

    /**
     * @brief 实体请求放手：先摘掉订阅索引里的条目，再放开持有记录。
     * @param entity 请求释放的实体。
     * @note  索引表只存弱引用，但残留的订阅仍会参与派发与状态展示，
     *        因此放手时必须一并摘除。
     */
    void on_disown(std::shared_ptr<Entity> entity) override;

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
     * @return 仍存活的监听者强引用列表（投递期间临时保命）；未注册时为空。
     * @note   已过期的监听者直接跳过；MVP 不做惰性清理。
     */
    std::vector<std::shared_ptr<Entity>> snapshot_listeners(std::size_t event_id) const;

    std::atomic<bool> running_{false}; ///< 事件循环是否继续

  private:
    // listeners_ 的一项：实体 + 它在 entity_events_ 中的槽位。
    struct Listener {
        std::weak_ptr<Entity> entity; ///< 不延长实体生命
        Entity*               raw = nullptr; ///< 仅用于比较 / 定位，不解引用
        std::size_t           sub_slot = 0;
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
