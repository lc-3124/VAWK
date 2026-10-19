#ifndef VAWK_ENTITY_STORE_HPP
#define VAWK_ENTITY_STORE_HPP

//- vawk::detail::EntityStore — 唯一强引用持有者
//
//  存储区只做三件事：接管实体的强引用、记录它在容器中的位置、按实体请求
//  松手。它不建 id / label 索引、不提供查找、不做任何生命周期决策。
//
//  用户通过 make_entity() 创建实体，只拿到 weak_ptr；所有 Holder 与
//  EventUpstream 也只持 weak_ptr。实体判定自己该走之后，调用
//  Entity::request_disown() 请求存储区松手，最后一份强引用消失时由
//  shared_ptr 正常析构。

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
 * @brief 创建实体：唯一强引用交给内部存储区，返回弱引用。
 * @tparam T 实体类型。
 * @param args 转发给 T 的构造参数。
 * @return 指向实体的 weak_ptr；实体释放后自动失效。
 */
template <class T, class... A>
std::weak_ptr<T> make_entity(A&&... args) {
    auto sp = std::make_shared<T>(std::forward<A>(args)...);
    detail::EntityStore::instance().attach(sp);
    return std::weak_ptr<T>(sp);
}

}  // namespace vawk

#endif  // VAWK_ENTITY_STORE_HPP
