#ifndef VAWK_EVENT_ROUTER_HPP
#define VAWK_EVENT_ROUTER_HPP

//- vawk::EventRouter — 带可推送缓冲区的具体 EventUpstream
//
//  EventRouter 是最简单的 EventUpstream 实现：事件被 push() 进内部队列，
//  dispatch_once() 出队后派发给订阅了该类型 ID 的实体。
//
//  典型用法：
//    - 创建一个 EventRouter 作为应用事件总线。
//    - 各 Entity 在 on_subscribe() 里向 router 订阅自己关心的事件类型。
//    - 任何线程都可以 push() 事件。
//    - 要么主线程手动 dispatch_once()，要么 start_event_loop() 后台投递。

#include "vawk/core/event_upstream.hpp"

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

namespace vawk {

/** @brief 最简 EventUpstream：事件入队后由 dispatch_once() 出队并派发。 */
class EventRouter : public EventUpstream {
  public:
    EventRouter()           = default;
    EventRouter(const EventRouter&)            = delete;
    EventRouter& operator=(const EventRouter&) = delete;

    /** @brief 析构：停掉事件循环。 */
    ~EventRouter() override { stop_event_loop(); }

    /**
     * @brief 将一个新事件推入内部缓冲区。
     * @param evt 待派发的事件。
     * @return 成功返回 true；evt 为空返回 false。
     */
    bool push(std::shared_ptr<event::Base> evt);

    /** @brief 取出一个事件，投递给所有订阅了该类型的监听者。 */
    void dispatch_once() override;

    /** @brief 后台事件循环：cv 等待队列，由 stop_event_loop() 停止。 */
    void dispatch_loop() override;

  protected:
    /** @brief 持队列锁 notify_all，避免循环恰好错过唤醒后永久阻塞。 */
    void wake_dispatch_thread() override;

  private:
    // 快照监听者后锁外投递，避免投递时持有订阅索引锁。
    void dispatch_event(std::shared_ptr<event::Base> evt);

    std::mutex                               mtx_router_queue_; ///< 保护 event_buffer_
    std::condition_variable                  cv_router_queue_; ///< 队列非空或停止
    std::deque<std::shared_ptr<event::Base>> event_buffer_; ///< 待派发事件队列
};

}  // namespace vawk

#endif  // VAWK_EVENT_ROUTER_HPP
