#ifndef VAWK_HOLDER_HPP
#define VAWK_HOLDER_HPP

//- vawk::Holder — 实体持有者基类
//
//  持有者（业务对象、EventUpstream 等）用 acquire() 在实体的“小本子”上
//  登记一条持有记录，用 release() 主动放手；析构时自动注销全部登记。
//
//  实体注销记录后若判定自己已成为孤儿（默认“只剩 Router”），会回调
//  holder_release() 请求持有者放手。默认实现只丢掉自己那条记录，不会
//  回头调用 Entity::remove_holder()，因此不会重入。

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace vawk {

class Entity;

/** @brief 持有者种类：参与实体的孤儿判定。 */
enum class HolderKind : std::uint8_t {
    Business = 0,  ///< 业务对象
    Router   = 1,  ///< 事件源（订阅关系）
    Parent   = 2,  ///< 父实体（弱引用语义）
    System   = 3,  ///< 框架内部
};

/**
 * @brief 实体持有者基类：登记 / 放手 / 析构自动注销。
 * @note  acquire() 与 release() 只维护持有记录；真正的销毁由持有者自己
 *        丢掉 shared_ptr 完成。
 */
class Holder {
  public:
    Holder()                         = default;
    Holder(const Holder&)            = delete;
    Holder& operator=(const Holder&) = delete;
    virtual ~Holder();

    /**
     * @brief 实体请求持有者放手时调用。
     * @param entity 请求释放的实体。
     * @note  默认实现只丢弃自己持有的那条记录，**不**回头调用
     *        Entity::remove_holder()，避免重入。
     * @note  重写实现可以在此丢掉业务侧强引用，从而让实体自然销毁。
     */
    virtual void holder_release(std::shared_ptr<Entity> entity);

  protected:
    /**
     * @brief 在实体小本子上登记一笔持有。
     * @param entity 被持有的实体。
     * @param kind   持有者种类。
     * @return 登记 token；同一持有者重复登记同一实体时返回既有 token，
     *         实体为空返回 0。
     * @note  同一持有者对同一实体只保留一条记录（按实体去重）。
     */
    std::uint64_t acquire(const std::shared_ptr<Entity>& entity, HolderKind kind);

    /**
     * @brief 主动放手：先删本地记录，再让实体注销对应条目。
     * @param entity 要放手的实体；未持有时什么都不做。
     */
    void release(const std::shared_ptr<Entity>& entity);

    /**
     * @brief 按裸指针放手（用于 upstream 的 detach 路径）。
     * @param entity 要放手的实体；未持有时什么都不做。
     */
    void release_by_ptr(const Entity* entity);

  private:
    // 本地持有记录；token 与实体小本子里的条目一一对应。
    struct Held {
        std::shared_ptr<Entity> entity;
        std::uint64_t           token = 0;
    };

    std::mutex        mtx_held_;
    std::vector<Held> held_;
};

}  // namespace vawk

#endif  // VAWK_HOLDER_HPP
