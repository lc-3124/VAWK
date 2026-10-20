#include "vawk/core/entity.hpp"
#include "vawk/core/entity_store.hpp"
#include "vawk/core/event_upstream.hpp"

#include <exception>
#include <utility>

namespace vawk {

//- 生命周期

Entity::Entity(std::shared_ptr<EventUpstream> upstream)
    : upstream_(std::move(upstream)) {}

Entity::~Entity() {
    shutdown_impl();
}

bool Entity::enable() {
    std::shared_ptr<Entity> self;
    {
        std::lock_guard lk(mtx_lifecycle_);
        if (enabled_.load()) return true;

        // 对象必须已由 shared_ptr 托管，否则 on_subscribe() 里取不到 self。
        auto up = upstream_.lock();
        if (!up) return false;
        if (weak_from_this().expired()) return false;

        if (!on_subscribe(*up)) {
            // 订阅中途失败：把已登记的条目全部回滚（含 Router 的持有记录）。
            up->detach(this);
            return false;
        }

        running_.store(true);
        worker_ = std::thread([this] { thread_main(); });
        enabled_.store(true);

        self = shared_from_this(); // 保命：判定与释放期间对象不能提前销毁
    }

    // 启用完成后立即判定：若已无 System 持有者，立刻请求 Router 放手。
    // 必须在 mtx_lifecycle_ 外判定：request_disown() 的回调可能反过来
    // 调 shutdown() 抢同一把锁。
    bool orphan = false;
    {
        std::lock_guard lk(mtx_holders_);
        orphan = release_policy_(build_holder_info_locked());
    }
    if (orphan) request_disown();

    return true;
}

void Entity::shutdown() {
    // 先取强引用：shutdown_impl() 期间本对象不会被析构掉。
    auto self = weak_from_this().lock();
    shutdown_impl();
    (void)self;
}

void Entity::shutdown_impl() {
    {
        std::lock_guard lk(mtx_lifecycle_);
        if (!enabled_.exchange(false)) return;
        // 持队列锁改 running_，与工作线程的谓词检查互斥，避免丢失唤醒。
        {
            std::lock_guard blk(mtx_eventbuffer_);
            running_.store(false);
        }
    }

    cv_eventbuffer_.notify_all();
    cv_space_.notify_all();

    if (worker_.joinable()) {
        // 在自己线程里销毁自身会导致 join 自己/随后 UAF，快速失败而不是静默 UB。
        if (worker_.get_id() == std::this_thread::get_id())
            std::terminate();
        worker_.join();
    }

    // 注销订阅并释放 Router 的持有记录；此时 enabled_ 已为 false，不会再触发
    // 孤儿判定，因此这条路径不会递归。
    if (auto up = upstream_.lock()) up->detach(this);
}

//- 持有协议

bool Entity::default_release_policy(const std::vector<HolderInfo>& holders) {
    for (const auto& h : holders)
        if (h.kind == HolderKind::System)
            return false;
    return true;
}

std::vector<HolderInfo> Entity::build_holder_info_locked() const {
    std::vector<HolderInfo> info;
    info.reserve(holders_.size());
    for (const auto& h : holders_)
        info.push_back({h.kind, h.owner});
    return info;
}

std::vector<HolderInfo> Entity::holder_snapshot() const {
    std::lock_guard lk(mtx_holders_);
    return build_holder_info_locked();
}

void Entity::set_release_policy(ReleasePolicy policy) {
    std::lock_guard lk(mtx_holders_);
    release_policy_ = policy
        ? std::move(policy)
        : ReleasePolicy(&Entity::default_release_policy);
}

void Entity::request_disown() {
    // 自保：回调过程中即使持有者丢掉全部引用，本对象也活到函数结束。
    auto self = shared_from_this();

    std::vector<Holder*> targets;
    {
        std::lock_guard lk(mtx_holders_);
        if (releasing_) return;
        releasing_ = true;
        targets.reserve(holders_.size());
        for (const auto& h : holders_) targets.push_back(h.owner);
        holders_.clear();
    }

    // 锁外逐个请求放手；默认实现只删自己的记录，不会回头重入本函数。
    for (auto* h : targets)
        if (h) h->on_disown(self);

    // 允许后续持有变化再次触发；本次若已真正析构则不会再执行到这里。
    {
        std::lock_guard lk(mtx_holders_);
        releasing_ = false;
    }

    // 实体已决定退出：先停线程并注销订阅。必须在存储区松手之前完成——
    // 最后一份强引用可能挂在 Router 的派发快照上，若直接放手，析构会发生在
    // 派发线程，与工作线程仍可能执行的虚调用 on_event() 竞争（vptr 改写）。
    shutdown_impl();

    // 请存储区丢掉最后一份强引用。self 仍按值持有本对象，因此 release()
    // 之后到函数返回之前始终安全。
    detail::EntityStore::instance().release(this);
}

//- 事件队列 / 工作线程

void Entity::push_event(std::shared_ptr<event::Base> evt) {
    std::unique_lock lk(mtx_eventbuffer_);
    cv_space_.wait(lk, [&] {
        return event_buffer_.size() < kMaxQueue || !running_.load();
    });
    if (!running_.load()) return;
    event_buffer_.push_back(std::move(evt));
    // 持锁通知：消费者可能正处于“判空 → 阻塞”之间，锁外通知会丢失。
    cv_eventbuffer_.notify_one();
}

void Entity::thread_main() {
    for (;;) {
        std::shared_ptr<event::Base> evt;
        {
            std::unique_lock lk(mtx_eventbuffer_);
            cv_eventbuffer_.wait(lk, [&] {
                return !event_buffer_.empty() || !running_.load();
            });
            if (!running_.load() && event_buffer_.empty()) break;
            evt = std::move(event_buffer_.front());
            event_buffer_.pop_front();
            // 空位通知同样持锁发出，避免生产者丢唤醒后永久阻塞。
            cv_space_.notify_one();
        }
        on_event(std::move(evt));
    }
}

}  // namespace vawk
