#ifndef _ENTITY_CONTROL_BLOCK_HPP_
#define _ENTITY_CONTROL_BLOCK_HPP_

#include <atomic>
#include <mutex>
#include <cassert>

namespace va {
// 前向声明：无需包含 VaEntity.hpp，避免依赖
class VaEntity;

class EntityControlBlock {
public:
    // 构造：接收 VaEntity 指针（仅指针，前向声明足够）
    explicit EntityControlBlock(VaEntity* entity);
    // 析构：不直接销毁实体（由强引用计数决定）
    ~EntityControlBlock();

    // 强引用计数操作
    void increment_strong();
    bool decrement_strong();  // 返回 true 表示强引用归零

    // 弱引用计数操作
    void increment_weak();
    bool decrement_weak();  // 返回 true 表示弱引用归零

    // 状态查询
    size_t strong_count() const;
    size_t weak_count() const;
    bool is_alive() const;  // 强引用 > 0 表示实体存活

    // 实体访问（线程安全）
    VaEntity* get_entity() const;
    // 销毁实体（仅强引用归零后调用）
    void destroy_entity();

private:
    VaEntity* entity_;                  // 管理的实体指针（前向声明支持）
    std::atomic_size_t strong_cnt_;     // 强引用计数（原子操作）
    std::atomic_size_t weak_cnt_;       // 弱引用计数（原子操作）
    mutable std::mutex mutex_;          // 保护实体指针访问（防止析构时访问）
};
}  // namespace va

#endif  // _ENTITY_CONTROL_BLOCK_HPP_

