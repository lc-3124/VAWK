#ifndef VAWK_ENTITY_STORE_HPP
#define VAWK_ENTITY_STORE_HPP

//- vawk::detail::EntityStore — 唯一强引用持有者
//
//  存储区只做三件事：接管实体的强引用、记录它在容器中的位置、按实体请求
//  松手。它不建 id / label 索引、不提供查找、不做任何生命周期决策。
//
//  业务通过 make_entity() 创建实体，拿到的是 EntityHandle<T>（一条持有契约），
//  不接触 weak_ptr；所有 Holder 与 EventUpstream 也只持 weak_ptr。实体判定
//  自己该走之后，调用 Entity::request_disown() 请求存储区松手，最后一份强
//  引用消失时由 shared_ptr 正常析构。

#include "vawk/core/entity_handle.hpp"

#include <cstddef>
#include <list>
#include <memory>
#include <mutex>
#include <utility>

namespace vawk {

class Entity;

namespace detail {

/** @brief 唯一强引用持有者：只保存，不查找、不决策。 */
class EntityStore {
  public:
    /** @brief 全局唯一实例（函数内静态，线程安全初始化）。 */
    static EntityStore& instance();

    /** @brief 接管一个实体的强引用，并记录它在容器中的位置。 */
    void attach(const std::shared_ptr<Entity>& entity);

    /** @brief 实体请求放手：移除强引用；锁外析构。 */
    void release(Entity* entity);

  private:
    EntityStore()  = default;
    ~EntityStore();

    std::mutex                         mtx_; ///< 保护 items_
    std::list<std::shared_ptr<Entity>> items_; ///< 唯一强引用所在
};

}  // namespace detail

/**
 * @brief 业务创建实体：唯一强引用交给内部存储区，返回绑定 Business 契约的句柄。
 * @tparam T 实体类型，须派生自 Entity。
 * @param args 转发给 T 的构造参数。
 * @return 已建立契约的 EntityHandle<T>；实体随后若自行释放，句柄会过期。
 * @note   创建后是否保活仍由孤儿判定决定（Business 不算保活）：实体 enable()
 *         成功且没有 System 持有者时会立刻请求放手。
 */
template <class T, class... A>
EntityHandle<T> make_entity(A&&... args) {
    auto sp = std::make_shared<T>(std::forward<A>(args)...);
    detail::EntityStore::instance().attach(sp);
    EntityHandle<T> handle;
    handle.hold(sp, HolderKind::Business);
    return handle;
}

}  // namespace vawk

#endif  // VAWK_ENTITY_STORE_HPP
