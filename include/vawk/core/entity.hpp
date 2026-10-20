#ifndef VAWK_ENTITY_HPP
#define VAWK_ENTITY_HPP

//- vawk::Entity — 接收事件的实体基类
//
//  实体类是消息系统中，消费者的基类。定义了消费者接收事件、订阅事件、消化事件
//  所需的基本方法，同时是“持有协议”的约束对象，从holder类派生。
//
//  实体类横跨了框架的两大基本设计：事件订阅与分发系统、生命周期管理系统。
//  关于生命周期管理，详见 holder.hpp
//  lc3124(261020): 这样设计让几个模块耦合度比较高，不够这是我现阶段想出的最好的解法了。

#include "vawk/core/entity_store.hpp"
#include "vawk/core/event.hpp"
#include "vawk/core/holder.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

/** @brief 每个实体事件队列的容量；满时阻塞上游。 */
#ifndef VAWK_ENTITY_QUEUE_SIZE
#define VAWK_ENTITY_QUEUE_SIZE 1024
#endif

namespace vawk {

// 定义见 event_upstream.hpp。
class EventUpstream;

/** @brief 一条持有关系的只读视图供查询与释放策略使用。 */
struct HolderInfo {
    HolderKind kind  = HolderKind::System; ///< 持有者身份
    Holder*    owner = nullptr; ///< 持有者实例
};

/**
 * @brief 接受事件的实体基类，包含实体一条队列和一条工作线程。
 * @note  一般而言，业务逻辑中，用 vawk::make_entity() 创建
 *  ，这样可以让唯一强引用由vawk内部存储区持有。
 *  enable() 与 shutdown() 需按序调用。
 * @warning 不要在 on_event() 回调里销毁自身。
 */
class Entity : public Holder, public std::enable_shared_from_this<Entity> {
  public:
    /**
     * @brief 构造：只保存 upstream 弱引用，不订阅、不起线程。
     * @param upstream 事件源；实体不持有其强引用。
     * @note lc3124(261020): 这个方法用于创建一个实体，但在创建初期，可以
     * 直接绑定需要的事件上游，具体绑定规则写在实体自身的一个虚函数中，也可以
     * 启动实体后再额外改写规则和添加对象。这个设计用于让实体类和上游类解耦，
     * 但尽可能不牺牲封装性。
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
     * @brief 停用并注销
     * @warning 若在工作线程on_event()内销毁自身，将直接 std::terminate。
     */
    void shutdown();

    /** @brief 是否已启用。 */
    bool enabled() const noexcept { return enabled_.load(); }

    /** @brief 人读标签，用于日志与状态展示。 */
    const std::string& label() const { return label_; }

    /** @brief 设置人读标签。 */
    void set_label(std::string label) { label_ = std::move(label); }

    /**
     * @brief 把事件放入本实体队列，由工作线程串行交给 on_event()。
     * @param evt 待处理事件。
     * @note  队列满时阻塞等待空位；实体已停用时直接丢弃。
     */
    void push_event(std::shared_ptr<event::Base> evt);

    /** @brief 队列容量上限，编译期可用 VAWK_ENTITY_QUEUE_SIZE 覆盖。 */
    static constexpr std::size_t kMaxQueue = VAWK_ENTITY_QUEUE_SIZE;

    /// 运行期释放策略：参数是当前持有关系列表，返回 true 表示可以释放。
    using ReleasePolicy = std::function<bool(const std::vector<HolderInfo>&)>;

    /**
     * @brief 设置释放策略；传空函数则恢复默认策略（无 System 持有者即可释放）。
     * @note 建议在 enable() 之前设置；与判定过程共用 mtx_holders_ 互斥。
     * @warning 策略在持有 mtx_holders_ 时调用：不要反向加锁（包括
     *          holder_snapshot()），也不要回调实体。
     * @note lc3124(261020): 实体会根据这个回调函数的逻辑来判断当前是否满足
     * 销毁前提，让生命周期的结束条件受编程控制。
     */
    void set_release_policy(ReleasePolicy policy);

    /**
     * @brief 导出当前持有者的只读快照。
     * @return 每条持有关系的 {身份, 实例}。
     * @note   owner 是裸指针，不延长持有者生命周期，仅用于身份比较。
     */
    std::vector<HolderInfo> holder_snapshot() const;

  protected:
    /**
     * @brief 批量订阅事件；由 enable() 在工作线程启动前调用。
     * @param up 事件源，保证非空。
     * @return 成功返回 true；返回 false 表示 enable() 失败并回滚。
     * @note lc3124(261020): 这个是可选的，设计此方法是为了允许实体
     * 提前写好订阅逻辑，实际上直接通过上游的subscibe方法和这个效果是一样的。
     */
    virtual bool on_subscribe(EventUpstream& up) = 0;

    /**
     * @brief 处理一个事件；在本实体工作线程中调用，且不持任何内部锁。
     * @param evt 待处理事件。
     * @note 需要使用者自己重写内部逻辑
     */
    virtual void on_event(std::shared_ptr<event::Base> evt) = 0;

    /**
     * @brief 机构内部创建：实体交给存储区，返回弱引用。
     * @tparam T 实体类型，须派生自 Entity。
     * @param args 转发给 T 的构造参数。
     * @return weak_ptr<T>，不建立任何契约；父组件需自行建立 System 契约
     *         （用 EntityHandle 或 Holder::acquire）来保活。
     * @note   仅派生类可调用：业务侧请用 vawk::make_entity() 拿 EntityHandle。
     * @warning 弱引用不保活：契约解除后随之过期，访问前必须 lock() 检查。
     */
    template <class T, class... A>
    static std::weak_ptr<T> make_internal(A&&... args) {
        auto sp = std::make_shared<T>(std::forward<A>(args)...);
        detail::EntityStore::instance().attach(sp);
        return std::weak_ptr<T>(sp);
    }

  private:
    friend class Holder;
    friend class detail::EntityStore;

    // 默认释放策略：没有任何 System 持有者即可释放（Router / Parent /
    // Business 都不算保活）。
    static bool default_release_policy(const std::vector<HolderInfo>& holders);

    std::vector<HolderInfo> build_holder_info_locked() const; ///< 调用方须持 mtx_holders_

    // 自保（shared_from_this）→ 锁内取走全部持有者 → 锁外逐个请求放手。
    void request_disown();

    // shutdown() 的实际实现；析构与 shutdown() 共用。
    void shutdown_impl();

    // 工作线程主体：cv 等待队列非空或停止，锁外调用 on_event()。
    void thread_main();

    struct HolderEntry {
        HolderKind kind  = HolderKind::System; ///< 持有者种类
        Holder*    owner = nullptr; ///< 持有者（不保存强引用）
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
    // 如果实体已经触发了释放，那么就不再允许上游再触发释放，否则可能造成释放循环。
    bool                                     releasing_  = false; ///< 是否已请求释放
    ReleasePolicy release_policy_ = &Entity::default_release_policy; ///< 释放策略；默认 = 无 System 持有者，可用 set_release_policy 覆盖；传空恢复默认
    std::string label_; ///< 人读标签

    // 存储区位置（唯一强引用在 detail::EntityStore 里）；未入册时 in_store_ 为 false。
    std::list<std::shared_ptr<Entity>>::iterator store_pos_;
    bool                                         in_store_ = false;
};

}  // namespace vawk

#endif  // VAWK_ENTITY_HPP
