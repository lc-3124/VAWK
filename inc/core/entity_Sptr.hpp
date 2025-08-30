#ifndef _ENTITY_SPTR_HPP_
#define _ENTITY_SPTR_HPP_

#include <cstddef>
#include <functional>
#include "EntityControlBlock.hpp"  // 依赖控制块声明
#include "enable_entity_shared_from_this.hpp"

namespace va {
// 前向声明：弱指针后续实现，避免循环
class entity_Wptr;

class entity_Sptr {
public:
    // 1. 基础构造
    explicit entity_Sptr(VaEntity* entity = nullptr);  // 从原始指针构造
    entity_Sptr(const entity_Sptr& other);             // 拷贝构造
    entity_Sptr(entity_Sptr&& other) noexcept;         // 移动构造
    explicit entity_Sptr(const entity_Wptr& weak);     // 从弱指针构造（后续实现弱指针后完善）
    ~entity_Sptr();                                    // 析构

    // 2. 赋值运算符
    entity_Sptr& operator=(const entity_Sptr& other);
    entity_Sptr& operator=(entity_Sptr&& other) noexcept;
    void reset(VaEntity* entity = nullptr);  // 重置指针（放弃所有权）

    // 3. 访问操作
    VaEntity& operator*() const;             // 解引用
    VaEntity* operator->() const;            // 成员访问
    VaEntity* get() const;                   // 获取原始指针
    explicit operator bool() const;          // 空指针判断

    // 4. 状态查询
    size_t use_count() const;                // 获取强引用计数
    void swap(entity_Sptr& other) noexcept;  // 交换指针

    // 5. 池化相关（后续实现 EntityPool 后完善）
    void release();

private:
    EntityControlBlock* control_block_;  // 指向控制块（核心）

    // 友元：允许弱指针访问控制块，允许控制块构造强指针
    friend class entity_Wptr;
    explicit entity_Sptr(EntityControlBlock* cb);  // 内部构造（供弱指针 lock() 使用）
};

// 辅助函数：交换两个强指针
void swap(entity_Sptr& a, entity_Sptr& b) noexcept;

// 比较运算符：比较指向的实体是否相同
bool operator==(const entity_Sptr& a, const entity_Sptr& b);
bool operator!=(const entity_Sptr& a, const entity_Sptr& b);

// 模板函数：创建 entity_Sptr 管理的 VaEntity 子类对象
// T：VaEntity 子类类型（如 EntityA、MyEntity）
// Args：构造函数参数类型（可变参数）
template <typename T, typename... Args>
entity_Sptr make_entity_sptr(Args&&... args) {
    // 编译期检查：确保 T 是 VaEntity 的派生类，避免错误类型
    static_assert(std::is_base_of<VaEntity, T>::value, 
            "make_entity_sptr: T must inherit from va::VaEntity");

    // 1. 用完美转发传递参数，创建 T 类型对象（new 裸指针仅在此处出现，外部不可见）
    // 2. 直接传递给 entity_Sptr 构造函数，由智能指针接管所有权
    return entity_Sptr(new T(std::forward<Args>(args)...));
}
}  // namespace va

namespace std
{
    template <>
        struct hash<va::entity_Sptr> {
            // 哈希函数：基于实体地址计算哈希值
            size_t operator()(const va::entity_Sptr& sptr) const noexcept {
                // 1. 获取实体的原始指针（空指针返回 0）
                va::VaEntity* entity = sptr.get();
                if (entity == nullptr) {
                    return 0;  // 空指针的哈希值固定为 0
                }
                // 2. 将指针转换为 size_t（地址值），作为哈希值
                // （或用 std::hash<void*> 计算指针的哈希，更通用）
                return std::hash<void*>()(static_cast<void*>(entity));
            }
        };

};
#endif  // _ENTITY_SPTR_HPP_

