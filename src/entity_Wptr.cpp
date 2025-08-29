#include "core/entity_Wptr.hpp"
#include "core/entity_Sptr.hpp"
#include <stdexcept>

namespace va {
// 1. 构造函数实现
entity_Wptr::entity_Wptr() noexcept : control_block_(nullptr) {}

// 从强指针构造：弱引用 +1
entity_Wptr::entity_Wptr(const entity_Sptr& strong) : control_block_(strong.control_block_) {
    if (control_block_ != nullptr) {
        control_block_->increment_weak();
    }
}

// 拷贝构造：弱引用 +1
entity_Wptr::entity_Wptr(const entity_Wptr& other) : control_block_(other.control_block_) {
    if (control_block_ != nullptr) {
        control_block_->increment_weak();
    }
}

// 移动构造：窃取所有权
entity_Wptr::entity_Wptr(entity_Wptr&& other) noexcept : control_block_(other.control_block_) {
    other.control_block_ = nullptr;
}

// 内部构造：从控制块构造
entity_Wptr::entity_Wptr(EntityControlBlock* cb) : control_block_(cb) {
    if (control_block_ != nullptr) {
        control_block_->increment_weak();
    }
}

// 析构：释放弱引用，必要时销毁控制块
entity_Wptr::~entity_Wptr() {
    reset();
}

// 2. 赋值运算符实现
entity_Wptr& entity_Wptr::operator=(const entity_Sptr& strong) {
    EntityControlBlock* new_cb = strong.control_block_;
    // 先加新控制块的弱引用
    if (new_cb != nullptr) {
        new_cb->increment_weak();
    }
    // 释放旧控制块
    reset();
    control_block_ = new_cb;
    return *this;
}

entity_Wptr& entity_Wptr::operator=(const entity_Wptr& other) {
    if (this != &other) {
        EntityControlBlock* new_cb = other.control_block_;
        if (new_cb != nullptr) {
            new_cb->increment_weak();
        }
        reset();
        control_block_ = new_cb;
    }
    return *this;
}

entity_Wptr& entity_Wptr::operator=(entity_Wptr&& other) noexcept {
    if (this != &other) {
        reset();
        control_block_ = other.control_block_;
        other.control_block_ = nullptr;
    }
    return *this;
}

// 重置弱指针
void entity_Wptr::reset() noexcept {
    EntityControlBlock* cb = control_block_;
    control_block_ = nullptr;

    if (cb != nullptr) {
        // 弱引用 -1，若归零且强引用已归零，则销毁控制块
        bool weak_zero = cb->decrement_weak();
        if (weak_zero && cb->strong_count() == 0) {
            delete cb;
        }
    }
}

// 3. 核心功能实现
// lock()：实体存活则返回强指针，否则返回空指针
entity_Sptr entity_Wptr::lock() const {
    if (expired()) {
        return entity_Sptr();  // 返回空强指针
    }
    // 调用强指针的内部构造（控制块已确保存活）
    return entity_Sptr(control_block_);
}

// 检查实体是否已销毁（强引用归零）
bool entity_Wptr::expired() const {
    return control_block_ == nullptr || !control_block_->is_alive();
}

// 获取弱引用计数
size_t entity_Wptr::use_count() const {
    return control_block_ != nullptr ? control_block_->weak_count() : 0;
}

void entity_Wptr::swap(entity_Wptr& other) noexcept {
    std::swap(control_block_, other.control_block_);
}

// 4. 重载 ->：等效于 lock()->（线程安全，过期抛异常）
entity_Sptr entity_Wptr::operator->() const {
    entity_Sptr sptr = lock();
    if (!sptr) {
        throw std::runtime_error("Accessing expired entity_Wptr");
    }
    return sptr;
}

// 辅助函数：交换
void swap(entity_Wptr& a, entity_Wptr& b) noexcept {
    a.swap(b);
}

// 比较运算符：比较指向的实体是否相同
bool operator==(const entity_Wptr& a, const entity_Wptr& b) {
    return a.lock().get() == b.lock().get();
}

bool operator!=(const entity_Wptr& a, const entity_Wptr& b) {
    return a.lock().get() != b.lock().get();
}
}  // namespace va

