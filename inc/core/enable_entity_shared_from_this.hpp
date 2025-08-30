#ifndef _ENABLE_ENTITY_SHARED_FROM_THIS_HPP_
#define _ENABLE_ENTITY_SHARED_FROM_THIS_HPP_

namespace va {
// 前向声明智能指针（无需包含头文件）
class entity_Sptr;
class entity_Wptr;

class enable_entity_shared_from_this {
protected:
    // 构造/析构：保护访问，仅子类可继承
    enable_entity_shared_from_this() noexcept;
    enable_entity_shared_from_this(const enable_entity_shared_from_this&) noexcept;
    enable_entity_shared_from_this& operator=(const enable_entity_shared_from_this&) noexcept;
    virtual ~enable_entity_shared_from_this() noexcept;  // 虚析构，确保子类正确销毁

public:
    // 核心方法：返回自身的强指针/弱指针
    entity_Sptr Sptr_from_this();
    entity_Wptr Wptr_from_this() const;
    entity_Wptr* weak_this_;

private:
    // 关键：用弱指针存储自身（指针类型，前向声明支持）

    // 友元：允许 entity_Sptr 初始化 weak_this_
    friend class entity_Sptr;
};
}  // namespace va

#endif  // _ENABLE_ENTITY_SHARED_FROM_THIS_HPP_

