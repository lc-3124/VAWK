#include "core/enable_entity_shared_from_this.hpp"
#include "core/entity_Sptr.hpp"
#include "core/entity_Wptr.hpp"
#include <stdexcept>

namespace va {
// 构造：初始化 weak_this_ 为 nullptr
enable_entity_shared_from_this::enable_entity_shared_from_this() noexcept
    : weak_this_(nullptr) {}

enable_entity_shared_from_this::enable_entity_shared_from_this(const enable_entity_shared_from_this&) noexcept
    : weak_this_(nullptr) {}  // 拷贝时不复制 weak_this_（避免多个实例共享）

enable_entity_shared_from_this& enable_entity_shared_from_this::operator=(const enable_entity_shared_from_this&) noexcept {
    return *this;  // 赋值时不修改 weak_this_
}

// 析构：释放 weak_this_ 指针（避免内存泄漏）
enable_entity_shared_from_this::~enable_entity_shared_from_this() noexcept {
    delete weak_this_;
}

// 返回自身强指针：通过 weak_this_ 锁定
entity_Sptr enable_entity_shared_from_this::Sptr_from_this() {
    if (weak_this_ == nullptr) {
        throw std::runtime_error("enable_entity_shared_from_this: not managed by entity_Sptr");
    }
    entity_Sptr locked = weak_this_->lock();
    if (!locked) {
        throw std::runtime_error("enable_entity_shared_from_this: entity already destroyed");
    }
    return locked;
}

// 返回自身弱指针：返回 weak_this_ 的拷贝
entity_Wptr enable_entity_shared_from_this::Wptr_from_this() const {
    if (weak_this_ == nullptr) {
        throw std::runtime_error("enable_entity_shared_from_this: not managed by entity_Sptr");
    }
    return *weak_this_;
}
}  // namespace va

