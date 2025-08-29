#include "core/entity_Sptr.hpp"
#include "core/entity_Wptr.hpp"  // 后续实现弱指针后添加
#include "core/VaEntity.hpp"

namespace va {
// entity_Sptr.cpp 中修改构造函数
entity_Sptr::entity_Sptr(VaEntity* entity) : control_block_(nullptr) {
    if (entity == nullptr) return;

    // 1. 新建控制块（原有逻辑）
    control_block_ = new EntityControlBlock(entity);

    // 2. 尝试转换为 enable_entity_shared_from_this 基类
    auto shared_base = dynamic_cast<enable_entity_shared_from_this*>(entity);
    if (shared_base != nullptr) {
        // 先释放原有 weak_this_（防止多次初始化）
        delete shared_base->weak_this_;
        // 用当前强指针构造弱指针，存入 shared_base
        shared_base->weak_this_ = new entity_Wptr(*this);
    }
}


// 拷贝构造：强引用 +1
entity_Sptr::entity_Sptr(const entity_Sptr& other) : control_block_(other.control_block_) {
    if (control_block_ != nullptr) {
        control_block_->increment_strong();
    }
}

// 移动构造：窃取所有权，原指针置空
entity_Sptr::entity_Sptr(entity_Sptr&& other) noexcept : control_block_(other.control_block_) {
    other.control_block_ = nullptr;
}

entity_Sptr::entity_Sptr(const entity_Wptr& weak) : control_block_(nullptr) {
    if (!weak.expired()) {
        control_block_ = weak.control_block_;
        control_block_->increment_strong();  // 强引用 +1
    }
}


// 内部构造：从控制块构造（供弱指针 lock() 使用）
entity_Sptr::entity_Sptr(EntityControlBlock* cb) : control_block_(cb) {
    if (control_block_ != nullptr) {
        control_block_->increment_strong();
    }
}

// 析构：释放强引用，必要时销毁实体和控制块
entity_Sptr::~entity_Sptr() {
    reset();
}

// 2. 赋值运算符实现
entity_Sptr& entity_Sptr::operator=(const entity_Sptr& other) {
    if (this != &other) {
        // 先加对方计数（防止自我赋值时提前销毁）
        EntityControlBlock* new_cb = other.control_block_;
        if (new_cb != nullptr) {
            new_cb->increment_strong();
        }

        // 释放当前所有权
        reset();
        control_block_ = new_cb;
    }
    return *this;
}

entity_Sptr& entity_Sptr::operator=(entity_Sptr&& other) noexcept {
    if (this != &other) {
        reset();
        control_block_ = other.control_block_;
        other.control_block_ = nullptr;
    }
    return *this;
}

// 重置：释放当前控制块，可选绑定新实体
void entity_Sptr::reset(VaEntity* entity) {
    EntityControlBlock* cb = control_block_;
    control_block_ = nullptr;

    if (cb != nullptr) {
        // 强引用 -1，若归零则销毁实体
        bool strong_zero = cb->decrement_strong();
        if (strong_zero) {
            cb->destroy_entity();
        }

        // 弱引用 -1，若归零则销毁控制块
        bool weak_zero = cb->decrement_weak();
        if (weak_zero) {
            delete cb;
        }
    }

    // 绑定新实体（若有）
    if (entity != nullptr) {
        control_block_ = new EntityControlBlock(entity);
    }
}

// 3. 访问操作实现
VaEntity& entity_Sptr::operator*() const {
    VaEntity* entity = get();
    if (entity == nullptr) {
        throw std::runtime_error("Dereferencing null entity_Sptr");
    }
    return *entity;
}

VaEntity* entity_Sptr::operator->() const {
    VaEntity* entity = get();
    if (entity == nullptr) {
        throw std::runtime_error("Accessing member of null entity_Sptr");
    }
    return entity;
}

VaEntity* entity_Sptr::get() const {
    return control_block_ != nullptr ? control_block_->get_entity() : nullptr;
}

entity_Sptr::operator bool() const {
    return get() != nullptr;
}

// 4. 状态查询实现
size_t entity_Sptr::use_count() const {
    return control_block_ != nullptr ? control_block_->strong_count() : 0;
}

void entity_Sptr::swap(entity_Sptr& other) noexcept {
    std::swap(control_block_, other.control_block_);
}

// 5. 池化 release 方法（暂时空实现，后续完善）
void entity_Sptr::release() {
    // 后续补充
}

// 辅助函数：交换
void swap(entity_Sptr& a, entity_Sptr& b) noexcept {
    a.swap(b);
}

// 比较运算符
bool operator==(const entity_Sptr& a, const entity_Sptr& b) {
    return a.get() == b.get();
}

bool operator!=(const entity_Sptr& a, const entity_Sptr& b) {
    return a.get() != b.get();
}
}  // namespace va

