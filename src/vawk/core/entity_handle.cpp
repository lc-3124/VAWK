#include "vawk/core/entity_handle.hpp"
#include "vawk/core/entity.hpp"

#include <mutex>
#include <utility>

namespace vawk {
namespace detail {

//- 契约内核
//
// 一条契约的全部状态都放在 EntityHandleCore 里，由句柄的 shared_ptr 独占、
// 地址恒定：实体小本子记的是 &EntityHandleCore，句柄被移动时只搬指针，那笔
// 登记仍然有效。
//
//  内核继承 Holder 复用 acquire() / disown()：
//    - bind()    → acquire()     在实体小本子上登记一笔（立刻可见）
//    - release() → disown()      两本账同时删掉这笔登记
//    - 实体请求放手 → on_disown() 默认实现清本地表，这里额外把 weak 置空，
//                                 让 expired() 立刻生效

class EntityHandleCore : public Holder {
  public:
    // 建立契约：只有第一次能成功，之后的调用一律拒绝。
    bool bind(const std::shared_ptr<Entity>& entity, HolderKind kind) {
        if (!entity) return false;

        {
            std::lock_guard lk(mtx_);
            if (bound_) return false;
            bound_  = true;
            entity_ = entity;
        }

        // 实体立刻记账；acquire() 内部用双锁同时更新两本账。
        acquire(entity, kind);
        return true;
    }

    // 解除契约：先摘 weak 再 disown，避免 disown() 触发的 on_disown()
    // 回头抢同一把 mtx_（这里不能在锁内 disown）。
    void release() {
        std::shared_ptr<Entity> entity;
        {
            std::lock_guard lk(mtx_);
            entity = entity_.lock();
            entity_.reset();
        }
        if (entity) disown(entity);
    }

    // 取访问用的临时强引用。
    std::shared_ptr<Entity> lock() const {
        std::lock_guard lk(mtx_);
        return entity_.lock();
    }

    // 实体请求放手：存储区即将松手，句柄这边同步失效。
    void on_disown(std::shared_ptr<Entity> entity) override {
        {
            std::lock_guard lk(mtx_);
            entity_.reset();
        }
        // 基类默认实现只清本地表，不会回头动实体的小本子（那边已清空）。
        Holder::on_disown(std::move(entity));
    }

  private:
    mutable std::mutex    mtx_;  ///< 保护 entity_ / bound_
    std::weak_ptr<Entity> entity_;  ///< 不延长实体生命
    bool                  bound_ = false;  ///< 是否已建立过契约（只允许一次）
};

//- 契约原语（句柄模板只调用这里，不复制逻辑）

std::shared_ptr<EntityHandleCore> entity_handle_make_core() {
    return std::make_shared<EntityHandleCore>();
}

bool entity_handle_bind(const std::shared_ptr<EntityHandleCore>& core,
                        const std::shared_ptr<Entity>&          entity,
                        HolderKind                              kind) {
    if (!core || !entity) return false;
    return core->bind(entity, kind);
}

void entity_handle_release(const std::shared_ptr<EntityHandleCore>& core) {
    if (core) core->release();
}

std::shared_ptr<Entity> entity_handle_lock(const std::shared_ptr<EntityHandleCore>& core) {
    return core ? core->lock() : nullptr;
}

}  // namespace detail
}  // namespace vawk
