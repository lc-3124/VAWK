#ifndef VAWK_EVENT_ROUTER_HPP
#define VAWK_EVENT_ROUTER_HPP

//- vawk::EventRouter — 带可推送缓冲区的具体 EventUpstream 实现
//
//  EventRouter 是最简单的 EventUpstream 实现：事件被推入内部队列，
//  dispatch_once() 将其出队并投递给已注册的监听者。
//
//  典型用法：
//    - 创建一个 EventRouter 作为应用程序的事件总线。
//    - 各 Entity 向 router 订阅自己关心的事件类型。
//    - 代码的任何部分都可以 push() 事件到 router。
//    - 要么在主线程手动调用 dispatch_once()，要么使用
//      start_event_loop() 进行后台投递。

#include "event_upstream.hpp"

#include <memory>
#include <queue>

namespace vawk {

/** @brief 最简 EventUpstream：事件入队后由 dispatch_once() 出队并派发。 */
class EventRouter : public EventUpstream {
  public:
    EventRouter() = default;
    ~EventRouter() override { stop_event_loop(); }

    /**
     * @brief 将一个新事件推入内部缓冲区。
     * @param evt 待派发的事件。
     * @return 成功返回 true（目前恒为 true）。
     */
    bool push(std::shared_ptr<event::Base> evt);

    /** @brief 取出一个事件，并投递给所有注册了该事件类型的监听者。 */
    void dispatch_once() override;

    /** @brief 后台事件循环：反复调用 dispatch_once()，由 stop_event_loop() 停止。 */
    void dispatch_loop() override;

  protected:
    std::queue<std::shared_ptr<event::Base>> event_buffer_; ///< 待派发事件队列
};

}  // namespace vawk

#endif  // VAWK_EVENT_ROUTER_HPP
