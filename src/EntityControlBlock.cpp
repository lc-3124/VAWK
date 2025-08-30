#include "core/EntityControlBlock.hpp"
// 在这里包含 VaEntity 完整定义（.cpp 层面，无依赖问题）
#include "core/VaEntity.hpp"

#include <iostream>

namespace va {
// 构造：强引用初始化为1，弱引用初始化为1（控制块自身持有1个弱引用）
EntityControlBlock::EntityControlBlock(VaEntity* entity)
    : entity_(entity), strong_cnt_(1), weak_cnt_(1) {
    assert(entity != nullptr && "Entity cannot be null (use empty entity_Sptr instead)");
}

// EntityControlBlock.cpp 中修改析构函数
EntityControlBlock::~EntityControlBlock() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entity_ != nullptr) {
        std::cerr << "[WARNING] Entity not destroyed before control block (fallback destroy)" << std::endl;
        // 处理 Raii() 异常，避免中断 delete
        try {
            entity_->Raii();
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception in entity Raii(): " << e.what() << std::endl;
        }
        // 无论是否异常，都要销毁实体并置空（避免双重释放）
        delete entity_;
        entity_ = nullptr;
    }
    assert(entity_ == nullptr && "Entity not destroyed before control block");
}


// 强引用 +1（线程安全，relaxed 内存序足够，无依赖）
void EntityControlBlock::increment_strong() {
    strong_cnt_.fetch_add(1, std::memory_order_relaxed);
}

// 强引用 -1（acq_rel 内存序：确保前序操作可见，后续操作不重排）
bool EntityControlBlock::decrement_strong() {
    if (strong_cnt_.fetch_sub(1, std::memory_order_seq_cst) == 1) {
        return true;  // 强引用归零
    }
    return false;
}

// 弱引用 +1（relaxed 内存序）
void EntityControlBlock::increment_weak() {
    weak_cnt_.fetch_add(1, std::memory_order_relaxed);
}

// 弱引用 -1（acq_rel 内存序）
bool EntityControlBlock::decrement_weak() {
    if (weak_cnt_.fetch_sub(1, std::memory_order_seq_cst) == 1) {
        return true;  // 弱引用归零
    }
    return false;
}

// 获取强引用计数（acquire 内存序：确保计数最新）
size_t EntityControlBlock::strong_count() const {
    return strong_cnt_.load(std::memory_order_acquire);
}

// 获取弱引用计数（acquire 内存序）
size_t EntityControlBlock::weak_count() const {
    return weak_cnt_.load(std::memory_order_acquire);
}

// 检查实体是否存活（强引用 > 0）
bool EntityControlBlock::is_alive() const {
    return strong_count() > 0;
}

// 线程安全获取实体指针（加锁防止析构时访问）
VaEntity* EntityControlBlock::get_entity() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return is_alive() ? entity_ : nullptr;
}

void EntityControlBlock::destroy_entity() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entity_) {
        try {
            entity_->Raii(); // 捕获Raii()可能抛出的异常
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] destroy_entity: exception in Raii() (" << e.what() << ")" << std::endl;
        }
        // 无论是否异常，强制销毁entity并置空（避免内存泄漏/野指针）
        delete entity_;
        entity_ = nullptr;
    }
}

}  // namespace va

