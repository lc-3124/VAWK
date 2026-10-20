#ifndef VAWK_ENTITY_HANDLE_HPP
#define VAWK_ENTITY_HANDLE_HPP

//- vawk::EntityHandle<T> — 业务侧实体句柄
//
//  业务的全部实体访问都经过本类，不接触 weak_ptr / shared_ptr：
//
//  一条契约 = 一个 EntityHandle。hold() 时句柄在实体小本子上登记一笔
//  （HolderKind::Business），访问期间内部临时 lock() 取一份强引用，
//  放掉即过期；release() / 析构时注销这笔登记。
//
//  句柄本身不保活：唯一强引用仍在 detail::EntityStore 里。契约解除后若实体
//  判定自己已成孤儿，会请求存储区松手，weak 随之过期——此后 expired() 为
//  true，with() 不再执行回调。
//
//  句柄内部用 pimpl（detail::EntityHandleCore）：实体小本子记的是持有者
//  **地址**，句柄若被移动（例如 push_back 进 std::vector），只有堆上的内核
//  地址稳定才能保证那笔登记仍然指向有效对象。
//
//  模板参数只决定 operator-> / get() / with() 的静态类型；契约逻辑由
//  非模板的 detail::EntityHandleCore 统一实现，不随 T 复制。

#include "vawk/core/holder.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace vawk {

class Entity;

namespace detail {

// 契约内核：非模板，所有 EntityHandle<T> 实例共用同一份实现。
class EntityHandleCore;

// 契约原语：定义在 entity_handle.cpp；模板只做类型转换。
std::shared_ptr<EntityHandleCore> entity_handle_make_core();

bool entity_handle_bind(const std::shared_ptr<EntityHandleCore>& core,
                        const std::shared_ptr<Entity>&          entity,
                        HolderKind                              kind);

void entity_handle_release(const std::shared_ptr<EntityHandleCore>& core);

std::shared_ptr<Entity> entity_handle_lock(const std::shared_ptr<EntityHandleCore>& core);

}  // namespace detail

/**
 * @brief 业务侧实体句柄：建立一条持有契约，并提供类型化访问。
 * @tparam T 实体类型，须派生自 Entity；默认 Entity 表示不做下转。
 * @note  不可拷贝；可移动。移动后源句柄为空（expired() 为 true）。
 * @note  一条句柄只能建立一个契约：release() 之后不可再用 hold() 复用。
 * @note  hold() 成功只代表登记成功，不代表实体此后一定存活；用 expired()
 *        或 with() 判断。
 * @note  get() / operator->() / with() / expired() 可并发调用；hold() 与
 *        release() 不要在同一个句柄上并发，二者不保证线性化。
 * @note  移动赋值时，被覆盖句柄原有的契约随旧 Core 析构而解除。
 * @warning operator->() 取到的裸指针只在当前完整表达式内有效；需要跨语句
 *          使用时改用 with()。
 */
template <class T = Entity>
class EntityHandle {
  public:
    EntityHandle() : core_(detail::entity_handle_make_core()) {}

    // 析构顺序即契约顺序：内核随句柄释放，~Holder 注销这笔登记。
    ~EntityHandle() = default;

    EntityHandle(const EntityHandle&)            = delete;
    EntityHandle& operator=(const EntityHandle&) = delete;
    EntityHandle(EntityHandle&& other) noexcept            = default;
    EntityHandle& operator=(EntityHandle&& other) noexcept = default;

    /**
     * @brief 建立契约：实体立刻记账；同一 handle 只能绑定一次。
     * @param entity 被持有的实体（shared_ptr 或派生类型）。
     * @param kind   持有者种类，默认 Business。
     * @return 成功 true；实体为空、句柄已绑定、或实体已失效返回 false。
     */
    bool hold(std::shared_ptr<T> entity, HolderKind kind = HolderKind::Business) {
        static_assert(std::is_base_of_v<Entity, T>, "T 必须是 Entity 的派生类");
        if (!entity) return false;
        // 被移动走的句柄重新成为空壳，但只能再建一次契约。
        if (!core_) core_ = detail::entity_handle_make_core();
        return detail::entity_handle_bind(core_, std::shared_ptr<Entity>(std::move(entity)),
                                          kind);
    }

    /**
     * @brief 建立契约（弱引用版本）。
     * @param entity 被持有的实体；已失效时返回 false。
     * @param kind   持有者种类，默认 Business。
     */
    bool hold(std::weak_ptr<T> entity, HolderKind kind = HolderKind::Business) {
        return hold(entity.lock(), kind);
    }

    /** @brief 主动解除契约（幂等）。 */
    void release() { detail::entity_handle_release(core_); }

    /**
     * @brief 访问期票：内部持有临时强引用，表达式结束即放掉。
     * @note  只在当前完整表达式内有效，不要跨语句保存。
     */
    class Access {
      public:
        explicit Access(std::shared_ptr<T> e) : held_(std::move(e)) {}
        T* operator->() const { return held_.get(); }
        T& operator*() const { return *held_; }
        explicit operator bool() const { return held_ != nullptr; }

      private:
        std::shared_ptr<T> held_;  ///< 访问期间的临时强引用
    };

    /** @brief 临时取一份强引用并解引用；实体不在时 Access 为空。 */
    Access operator->() const { return get(); }

    /** @brief 同 operator->()，语义更明确。 */
    Access get() const { return Access(lock_()); }

    /**
     * @brief 实体在则执行回调；不在返回 false。
     * @param fn 接受 T& 的可调用对象；回调期间实体保持存活。
     */
    template <class F>
    bool with(F&& fn) const {
        if (auto e = lock_()) {
            fn(*e);
            return true;
        }
        return false;
    }

    /** @brief 实体是否已失效（含未绑定 / 已被请求放手 / 已移动走）。 */
    bool expired() const { return !lock_base_(); }

    /** @brief 等同于 !expired()。 */
    explicit operator bool() const { return !expired(); }

  private:
    // 取一份临时强引用并按 T 下转；无实体或已过期时为空。
    std::shared_ptr<T> lock_() const {
        return std::static_pointer_cast<T>(lock_base_());
    }

    std::shared_ptr<Entity> lock_base_() const { return detail::entity_handle_lock(core_); }

    std::shared_ptr<detail::EntityHandleCore> core_;  ///< 堆上内核，地址稳定
};

}  // namespace vawk

#endif  // VAWK_ENTITY_HANDLE_HPP
