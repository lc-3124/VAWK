#include "core/entity_Sptr.hpp"
#include "core/entity_Wptr.hpp"  // 后续实现弱指针后添加
#include "core/VaEntity.hpp"

namespace va {
// entity_Sptr.cpp 中修改构造函数
entity_Sptr::entity_Sptr(VaEntity* entity) : control_block_(nullptr) {
    if (entity == nullptr) return;

    control_block_ = new EntityControlBlock(entity);
    auto shared_base = dynamic_cast<enable_entity_shared_from_this*>(entity);
    if (shared_base != nullptr) {
        // 安全初始化：先释放旧指针（即使为 nullptr，delete 也安全）
        delete shared_base->weak_this_;
        // 确保 new 成功（若内存不足，会抛出 bad_alloc，避免空指针）
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
// 移动赋值
entity_Sptr& entity_Sptr::operator=(entity_Sptr&& other) noexcept {
    if (this != &other) {
        reset(); // 先释放当前控制块
        control_block_ = other.control_block_;
        other.control_block_ = nullptr; // 关键：置空原指针
    }
    return *this;
}

// entity_Sptr.cpp 中修改 reset()
void entity_Sptr::reset(VaEntity* entity) {
    EntityControlBlock* cb = control_block_;
    control_block_ = nullptr;
    
    if (cb != nullptr) {
        // 步骤1：强引用计数减1，用返回值判断是否归零（无竞态）
        bool strong_zero = cb->decrement_strong();
        // 强引用归零必须调用 destroy_entity()，确保实体先销毁
        if (strong_zero) {
            cb->destroy_entity(); // 正常流程销毁实体，避免兜底逻辑
        }

        // 步骤2：弱引用计数减1，销毁控制块（此时实体已安全销毁）
        bool weak_zero = cb->decrement_weak();
        if (weak_zero) {
            delete cb; // 控制块析构时，entity_ 已为 nullptr，无警告
        }
    }

    // 绑定新实体（原逻辑不变）
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

