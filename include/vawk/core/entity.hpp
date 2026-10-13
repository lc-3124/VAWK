#ifndef VAWK_ENTITY_HPP
#define VAWK_ENTITY_HPP

// -----------------------------------------------------------------------
//  vawk::Entity — 接收事件的实体基类
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
// -----------------------------------------------------------------------

#include "event.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

namespace vawk {

// 前向声明 —— 定义见 event_upstream.hpp。
class EventUpstream;

class Entity {
  public:
    Entity() = default;
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    virtual ~Entity();

    // ── 事件缓冲 ────────────────────────────────────────────────────────

    // 把事件推入线程安全的内部队列。
    // 之后会通过 on_event() 投递该事件。
    void push_event(std::shared_ptr<event::Base> evt);

    enum class ProcessResult { Processed, Discarded, Empty };
    ProcessResult process_one_event();

    // ── 订阅辅助 ────────────────────────────────────────────────────────

    // 从某个 upstream 订阅一个或多个事件类型 ID。
    void subscribe(EventUpstream* upstream, std::vector<size_t> event_ids);

    // 取消订阅某个 upstream 上的指定事件类型 ID。
    void unsubscribe(EventUpstream* upstream, std::vector<size_t> event_ids);

    // 取消订阅给定 upstream 上的所有事件类型。
    void unsubscribe(EventUpstream* upstream);

    // ── 标签 ────────────────────────────────────────────────────────────

    std::string label() const { return label_; }
    void set_label(std::string l) { label_ = std::move(l); }

  protected:
    // 重写此方法以处理推入缓冲的事件。
    virtual void on_event(std::shared_ptr<event::Base> evt) = 0;

    // ── 内部状态 ───────────────────────────────────────────────────────

    std::mutex mutex_;
    std::queue<std::shared_ptr<event::Base>> event_buffer_;

    // 本实体已订阅的 upstream 列表（用于批量
    // 取消订阅）。
    std::vector<EventUpstream*> upstreams_;
    std::string label_;
};

}  // namespace vawk

#endif  // VAWK_ENTITY_HPP
