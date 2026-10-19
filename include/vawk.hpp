#ifndef VAWK_HPP
#define VAWK_HPP

//- vawk — Visual Ansi Widget Kit（核心事件框架）
//
//  在 vaterm 终端库之上，提供一套用于构建 TUI 应用的事件驱动框架。
//
//  架构总览：
//
//    vawk::event::Base   <──  用户定义的事件结构体
//         │                       (VAWK_EVENT_DEFINE 宏)
//         │
//         ├── Entity            ──  接收事件（重写 on_subscribe / on_event）
//         │
//         ├── EventUpstream     ──  事件源，向各实体路由事件
//         │       │
//         │       └── EventRouter  具体的 upstream，带可 push 的缓冲区
//         │
//         └── Input (占位)        读取终端输入 → 事件
//
//    vawk::Holder        ──  持有者基类（acquire / disown / on_disown）
//         ├── Entity
//         └── EventUpstream
//
//  典型用法：
//    1. 用 VAWK_EVENT_DEFINE / VAWK_EVENT_DEFINE_END 定义事件类型。
//    2. 创建 EventRouter 作为中心事件总线。
//    3. 继承 Entity，重写 on_subscribe() 与 on_event()。
//    4. 用 vawk::make_entity<T>() 创建实体（只拿到 weak_ptr），
//       调用 enable() 订阅并起线程。
//    5. 把事件 push 进 router；实体在自己的线程里收到事件。
//
//  唯一强引用由内部 detail::EntityStore 持有；Holder 与 upstream 都只持
//  弱引用，实体判定自己该走之后由存储区松手销毁。
//
//  示例：
//    #include <vawk.hpp>
//
//    VAWK_EVENT_DEFINE(MyEvent) int value; VAWK_EVENT_DEFINE_END
//        ↑ 宏体内已用 vawk:: 限定 event_type_id，事件类型可定义在任意作用域。
//
//    class MyEntity : public vawk::Entity {
//      public:
//        using vawk::Entity::Entity;
//
//      protected:
//        bool on_subscribe(vawk::EventUpstream& up) override {
//            up.subscribe(vawk::event_type_id<MyEvent>(), shared_from_this());
//            return true;
//        }
//
//        void on_event(std::shared_ptr<vawk::event::Base> evt) override {
//            if (auto* e = vawk::event::cast<MyEvent>(*evt)) {
//                printf("got value %d\n", e->value);
//            }
//        }
//    };

#include "vawk/core/holder.hpp"
#include "vawk/core/event.hpp"
#include "vawk/core/entity.hpp"
#include "vawk/core/entity_store.hpp"
#include "vawk/core/event_upstream.hpp"
#include "vawk/core/event_router.hpp"
#include "vawk/core/input.hpp"
#include "vawk/core/widget.hpp"
#include "vawk/widget/window.hpp"
#include "vawk/widget/drawable.hpp"

#endif  // VAWK_HPP
