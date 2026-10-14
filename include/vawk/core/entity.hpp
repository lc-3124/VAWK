#ifndef VAWK_ENTITY_HPP
#define VAWK_ENTITY_HPP

//- vawk::Entity — 接收事件的实体基类
//
//  Entity 提供：
//    - 线程安全的事件缓冲（push_event / process_one_event）。
//    - 便捷的订阅辅助（subscribe / unsubscribe）。
//    - 一个可选的、便于人读的标签（set_label / label）。
//
//  继承 Entity 并重写受保护的 on_event() 方法，即可处理传入的事件。
//
//  订阅通过指向 EventUpstream 的裸指针完成（而非 shared_from_this），
//  以避开原设计里的 std::bad_weak_ptr 陷阱。调用方必须保证 Entity
//  比它所订阅的各个 upstream 活得更久。

#include "event.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace vawk {

// 前向声明 —— 定义见 event_upstream.hpp。
class EventUpstream;

/**
 * @brief 接收事件的实体基类。
 * @note 订阅用裸指针指向 EventUpstream（避免 shared_from_this 的
 *       bad_weak_ptr）。调用方必须保证 Entity 比其 upstream 活得更久。
 */
class Entity {
  public:
    Entity() = default;
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    virtual ~Entity();

    //-- 事件缓冲

    /**
     * @brief 把事件推入线程安全的内部队列。
     * @note 之后会通过 on_event() 投递该事件。
     * @param evt 待投递的事件。
     */
    void push_event(std::shared_ptr<event::Base> evt);

    /** @brief process_one_event() 的结果码。 */
    enum class ProcessResult { Processed, Discarded, Empty };

    /**
     * @brief 从内部队列取出并投递一个事件。
     * @return Processed 已投递；Discarded 已丢弃；Empty 队列为空。
     */
    ProcessResult process_one_event();

    //-- 订阅辅助

    /**
     * @brief 向某个 upstream 订阅一个或多个事件类型 ID。
     * @param upstream  事件源。
     * @param event_ids 订阅的事件类型 ID 列表。
     */
    void subscribe(EventUpstream* upstream, std::vector<size_t> event_ids);

    /**
     * @brief 取消订阅某个 upstream 上的指定事件类型 ID。
     * @param upstream  事件源。
     * @param event_ids 要注销的事件类型 ID 列表。
     */
    void unsubscribe(EventUpstream* upstream, std::vector<size_t> event_ids);

    /**
     * @brief 取消订阅给定 upstream 上的所有事件类型。
     * @param upstream 事件源。
     */
    void unsubscribe(EventUpstream* upstream);

    //-- 标签

    /** @brief 取人读标签。 */
    std::string label() const { return label_; }

    /** @brief 设置人读标签。 */
    void set_label(std::string l) { label_ = std::move(l); }

  protected:

    /**
     * @brief 处理推入缓冲的事件；子类重写此方法。
     * @param evt 待处理的事件。
     */
    virtual void on_event(std::shared_ptr<event::Base> evt) = 0;

    //-- 内部状态

    std::mutex mutex_;
    std::queue<std::shared_ptr<event::Base>> event_buffer_;

    // 本实体已订阅的 upstream 列表（用于批量取消订阅）。
    std::vector<EventUpstream*> upstreams_;
    std::string label_;
};

}  // namespace vawk

#endif  // VAWK_ENTITY_HPP
