#ifndef VAWK_HPP
#define VAWK_HPP

// -----------------------------------------------------------------------
//  vawk — Visual Ansi Widget Kit（核心事件框架）
//
//  在 vaterm 终端库之上，提供一套用于构建 TUI 应用的事件驱动框架。
//
//  架构总览：
//
//    vawk::event::Base   <──  用户定义的事件结构体
//         │                       (VAWK_EVENT_DEFINE 宏)
//         │
//         ├── Entity            ──  接收事件（重写 on_event）
//         │
//         ├── EventUpstream     ──  事件源，向各实体路由事件
//         │       │
//         │       └── EventRouter  具体的 upstream，带可 push 的缓冲区
//         │
//         └── Input (占位)        读取终端输入 → 事件
//
//  典型用法：
//    1. 用 VAWK_EVENT_DEFINE / VAWK_EVENT_DEFINE_END 定义事件类型。
//    2. 创建 EventRouter 作为中心事件总线。
//    3. 继承 Entity，重写 on_event()，并订阅需要的事件。
//    4. 把事件 push 进 router；实体通过 dispatch 收到事件。
//
//  示例：
//    #include <vawk.hpp>
//
//    VAWK_EVENT_DEFINE(MyEvent) int value; VAWK_EVENT_DEFINE_END
//
//    class MyEntity : public vawk::Entity {
//      void on_event(std::shared_ptr<vawk::event::Base> evt) override {
//          if (auto* e = vawk::event::cast<MyEvent>(*evt)) {
//              printf("got value %d\n", e->value);
//          }
//      }
//    };
// -----------------------------------------------------------------------

#include "vawk/event.hpp"
#include "vawk/entity.hpp"
#include "vawk/event_upstream.hpp"
#include "vawk/event_router.hpp"
#include "vawk/input.hpp"
#include "vawk/widget.hpp"
#include "vawk/window.hpp"
#include "vawk/drawable.hpp"

#endif  // VAWK_HPP
