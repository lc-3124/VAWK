#ifndef _ENTITY_WPTR_HPP_
#define _ENTITY_WPTR_HPP_

#include <cstddef>
#include "EntityControlBlock.hpp"

namespace va {
// 前向声明强指针
class entity_Sptr;

class entity_Wptr {
public:
    // 1. 基础构造
    entity_Wptr() noexcept;                      // 默认构造（空指针）
    entity_Wptr(const entity_Sptr& strong);      // 从强指针构造（核心）
    entity_Wptr(const entity_Wptr& other);       // 拷贝构造
    entity_Wptr(entity_Wptr&& other) noexcept;   // 移动构造
    ~entity_Wptr();                              // 析构

    // 2. 赋值运算符
    entity_Wptr& operator=(const entity_Sptr& strong);
    entity_Wptr& operator=(const entity_Wptr& other);
    entity_Wptr& operator=(entity_Wptr&& other) noexcept;
    void reset() noexcept;  // 重置弱指针

    // 3. 核心功能
    entity_Sptr lock() const;  // 获取强指针（实体存活则返回有效指针）
    bool expired() const;       // 检查实体是否已销毁
    size_t use_count() const;   // 获取弱引用计数
    void swap(entity_Wptr& other) noexcept;  // 交换

    // 4. 便捷访问：重载 ->，等效于 lock()->
    entity_Sptr operator->() const;

private:
    EntityControlBlock* control_block_;  // 指向控制块（与强指针共享）

    // 友元：允许强指针访问控制块
    friend class entity_Sptr;
    explicit entity_Wptr(EntityControlBlock* cb);  // 内部构造
};

// 辅助函数：交换两个弱指针
void swap(entity_Wptr& a, entity_Wptr& b) noexcept;

// 比较运算符
bool operator==(const entity_Wptr& a, const entity_Wptr& b);
bool operator!=(const entity_Wptr& a, const entity_Wptr& b);
}  // namespace va

#endif  // _ENTITY_WPTR_HPP_

