#ifndef VAWK_ENTITY_HPP
#define VAWK_ENTITY_HPP

//- vawk::Entity — 接收事件的实体基类
//
//  两阶段生命周期：
//
//    构造         只记下 upstream 的 weak_ptr，不登记、不起线程。
//    enable()     校验 weak_from_this() 有效 → on_subscribe() 订阅 →
//                 失败则 up->detach(this) 回滚 → 成功则启动每实体线程。
//    shutdown()   幂等：置停止标志、唤醒、join、detach()。
//
//  持有协议见 holder.hpp：持有者用 acquire() 在实体的小本子上登记；
//  每注销一笔，实体检查自己是否成为孤儿，是则请求所有持有者放手。
//
//  实体必须由 std::shared_ptr 托管；upstream 只存 weak_ptr，避免
//  “实体 ↔ 事件源”互相强引用形成环。

#include "vawk/core/event.hpp"
#include "vawk/core/holder.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

/** @brief 每个实体事件队列的容量；满时生产者阻塞。 */
#ifndef VAWK_ENTITY_QUEUE_SIZE
#define VAWK_ENTITY_QUEUE_SIZE 1024
#endif

namespace vawk {

// 定义见 event_upstream.hpp。
class EventUpstream;

/**
 * @brief 接收事件的实体基类：每实体一条队列 + 一条工作线程。
 * @note  必须由 std::shared_ptr 托管；enable() 与 shutdown() 需按序调用。
 * @warning 不要在 on_event() 回调里销毁自身。
 */
class Entity : public Holder, public std::enable_shared_from_this<Entity> {
  public:
    /**
     * @brief 构造：只保存 upstream 弱引用，不订阅、不起线程。
     * @param upstream 事件源；实体不持有其强引用。
     */
    explicit Entity(std::shared_ptr<EventUpstream> upstream);

    /** @brief 析构：等价于 shutdown()，确保工作线程退出并注销订阅。 */
    ~Entity() override;

    Entity(const Entity&)            = delete;
    Entity& operator=(const Entity&) = delete;

    /**
     * @brief 启用实体：订阅事件并启动工作线程。
     * @return 成功返回 true；upstream 已失效、对象不由 shared_ptr 托管
     *         或 on_subscribe() 失败时返回 false。
     * @note  失败时调用 upstream->detach(this) 回滚已登记的订阅。
     */
    bool enable();

    /**
     * @brief 停用并注销：幂等。
     * @warning 若在工作线程（on_event()）内销毁自身，将直接 std::terminate。
     */
    void shutdown();

    /** @brief 是否已启用。 */
    bool enabled() const noexcept { return enabled_.load(); }

    /**
     * @brief 把事件放入本实体队列，由工作线程串行交给 on_event()。
     * @param evt 待处理事件。
     * @note  队列满时阻塞等待空位；实体已停用时直接丢弃。
     */
    void push_event(std::shared_ptr<event::Base> evt);

    /** @brief 队列容量上限（编译期可用 VAWK_ENTITY_QUEUE_SIZE 覆盖）。 */
    static constexpr std::size_t kMaxQueue = VAWK_ENTITY_QUEUE_SIZE;

    /// 运行期释放策略：参数是当前持有者身份列表，返回 true 表示可以释放。
    using ReleasePolicy = std::function<bool(const std::vector<HolderKind>&)>;

    /**
     * @brief 设置释放策略；传空函数则恢复默认的 `is_orphan()` 判定。
     * @note 建议在 enable() 之前设置；与判定过程共用 mtx_holders_ 互斥。
     */
    void set_release_policy(ReleasePolicy policy);

  protected:
    /**
     * @brief 订阅事件；由 enable() 在工作线程启动前调用。
     * @param up 事件源，保证非空。
     * @return 成功返回 true；返回 false 表示 enable() 失败并回滚。
     */
    virtual bool on_subscribe(EventUpstream& up) = 0;

    /**
     * @brief 处理一个事件；在本实体工作线程中调用，且不持任何内部锁。
     * @param evt 待处理事件。
     */
    virtual void on_event(std::shared_ptr<event::Base> evt) = 0;

    /**
     * @brief 孤儿判定：每注销一笔持有后调用一次。
     * @return 默认实现：无持有者，或只剩 Router 时返回 true。
     * @note  调用时持有 mtx_holders_，重写实现不要反向加锁。
     */
    virtual bool is_orphan() const;

  private:
    friend class Holder;

    // 在小本子上登记一笔持有；由 Holder::acquire() 调用。
    std::uint64_t add_holder(HolderKind kind, Holder* owner);

    // 注销指定 token；若已启用且因此成为孤儿，则请求释放。
    void remove_holder(std::uint64_t token);

    // 自保（shared_from_this）→ 锁内取走全部持有者 → 锁外逐个请求放手。
    void request_disown();

    // shutdown() 的实际实现；析构与 shutdown() 共用。
    void shutdown_impl();

    // 工作线程主体：cv 等待队列非空或停止，锁外调用 on_event()。
    void thread_main();

    struct HolderEntry {
        std::uint64_t token = 0; ///< 单调递增的登记号
        HolderKind    kind  = HolderKind::System; ///< 持有者种类
        Holder*       owner = nullptr; ///< 持有者（不保存强引用）
    };

    std::weak_ptr<EventUpstream> upstream_; ///< 事件源弱引用（防环）
    std::atomic<bool>            enabled_{false}; ///< 是否已启用
    std::atomic<bool>            running_{false}; ///< 工作线程是否继续

    mutable std::mutex mtx_holders_; ///< 保护 holders_
    std::mutex         mtx_eventbuffer_; ///< 保护 event_buffer_ 与 running_
    std::mutex         mtx_lifecycle_; ///< 串行化 enable / shutdown

    std::condition_variable cv_eventbuffer_; ///< 队列非空或停止
    std::condition_variable cv_space_; ///< 队列出现空位

    std::vector<HolderEntry>                 holders_; ///< 持有者小本子
    std::deque<std::shared_ptr<event::Base>> event_buffer_; ///< 事件队列
    std::thread                              worker_; ///< 工作线程
    std::uint64_t                            next_token_ = 0; ///< token 发号器
    // 如果实体已经触发了释放，那么就不再允许上游再触发释放，否则可能造成释放循环。
    bool                                     releasing_  = false; ///< 是否已请求释放
    ReleasePolicy release_policy_; ///< 可选：运行期释放策略（空 = 用 is_orphan）
};

}  // namespace vawk

#endif  // VAWK_ENTITY_HPP
