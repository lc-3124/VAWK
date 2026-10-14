#ifndef VAWK_EVENT_HPP
#define VAWK_EVENT_HPP

//- vawk::event — 事件类型系统
//
//  每个事件都继承自 vawk::event::Base，后者提供虚函数 id()，
//  返回每个类型唯一的 size_t。类型 ID 在运行时由全局原子计数器
//  生成，首次访问 event_type_id<T>() 时触发分配。
//
//  宏：
//    VAWK_EVENT_DEFINE(Name) ... VAWK_EVENT_DEFINE_END
//        定义 struct Name : Base，并带上正确的 id() 重写。
//
//  自由函数：
//    event::is<T>(Base&)   — 事件类型为 T 时返回 true
//    event::cast<T>(Base&) — 向下转型；类型不匹配返回 nullptr

#include <atomic>
#include <cstddef>
#include <cstdlib>

namespace vawk {

//- 类型 ID 生成

/** @brief 用于给事件类型分配唯一 ID 的全局计数器。 */
inline std::atomic<size_t>& global_event_id_counter() {
    static std::atomic<size_t> counter{0};
    return counter;
}

/**
 * @brief 返回 T 的类型 ID（每处实例化唯一）。
 * @note 每个实例化在程序启动时取得下一个计数值（由函数内 static 的初始化保证）。
 * @return 该事件类型的唯一 ID。
 */
template <typename T>
size_t event_type_id() {
    static size_t id = global_event_id_counter()++;
    return id;
}

//- 定义事件类型的宏

/** @brief 向继承自 vawk::event::Base 的结构体注入必需的 id() 重写。 */
#define VAWK_EVENT_BASE_METHOD(T)      \
    size_t id() const override {       \
        return event_type_id<T>();     \
    }

/**
 * @brief 定义一个新的事件类型。
 *
 * 用法：
 *   VAWK_EVENT_DEFINE(MouseEvent)
 *       int x, y;
 *   VAWK_EVENT_DEFINE_END
 */
#define VAWK_EVENT_DEFINE(EventName)                \
    struct EventName : public vawk::event::Base {   \
        VAWK_EVENT_BASE_METHOD(EventName)

/** @brief 结束 VAWK_EVENT_DEFINE 定义的结构体。 */
#define VAWK_EVENT_DEFINE_END \
    };

//- event 命名空间

namespace event {

/**
 * @brief 系统中所有事件的基类。
 * @note 每个具体事件类型都必须继承 Base 并重写 id()。
 */
struct Base {
    virtual ~Base() = default;
    virtual size_t id() const = 0;
};

/**
 * @brief 运行时类型检查：e 是否为 T 的实例。
 * @tparam T 目标事件类型。
 * @param e  事件基类引用。
 * @return 类型匹配返回 true。
 */
template <typename T>
bool is(const Base& e) {
    return e.id() == event_type_id<T>();
}

/**
 * @brief 向下转型：若 e 是 T，返回指向 T 部分的指针；否则返回 nullptr。
 * @tparam T 目标事件类型。
 * @param e  事件基类引用。
 * @return 类型匹配返回 T*，否则 nullptr。
 */
template <typename T>
const T* cast(const Base& e) {
    if (is<T>(e)) {
        return &static_cast<const T&>(e);
    }
    return nullptr;
}

}  // namespace event
}  // namespace vawk

#endif  // VAWK_EVENT_HPP
