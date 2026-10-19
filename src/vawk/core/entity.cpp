#include "vawk/core/entity.hpp"
#include "vawk/core/event_upstream.hpp"

#include <algorithm>
#include <utility>

namespace vawk {

//- 生命周期

Entity::Entity(std::shared_ptr<EventUpstream> upstream)
    : upstream_(std::move(upstream)) {}

Entity::~Entity() {
    release_impl();
}

bool Entity::enable() {
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
    return true;
}

void Entity::release() {
    // 先取强引用：release_impl() 期间本对象不会被析构掉。
    auto self = weak_from_this().lock();
    release_impl();
    (void)self;
}

void Entity::release_impl() {
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
        if (worker_.get_id() == std::this_thread::get_id())
            worker_.detach();  // 工作线程内调用：不能自我 join
        else
            worker_.join();
    }

    // 注销订阅并释放 Router 的持有记录；此时 enabled_ 已为 false，不会再触发
    // 孤儿判定，因此这条路径不会递归。
    if (auto up = upstream_.lock()) up->detach(this);
}

//- 持有协议

std::uint64_t Entity::add_holder(HolderKind kind, Holder* owner) {
    std::lock_guard lk(mtx_holders_);
    const auto token = ++next_token_;
    holders_.push_back({token, kind, owner});
    return token;
}

void Entity::remove_holder(std::uint64_t token) {
    bool orphan = false;
    {
        std::lock_guard lk(mtx_holders_);
        auto it = std::find_if(holders_.begin(), holders_.end(),
                               [&](const HolderEntry& h) { return h.token == token; });
        if (it == holders_.end()) return;
        if (it != holders_.end() - 1) *it = holders_.back();
        holders_.pop_back();
        if (enabled_.load()) orphan = is_orphan();
    }
    // 判定与回调都在锁外收尾，持有者可以安全地反向操作实体。
    if (orphan) request_release();
}

bool Entity::is_orphan() const {
    // 调用方持有 mtx_holders_。
    if (holders_.empty()) return true;
    if (holders_.size() != 1) return false;
    return holders_.front().kind == HolderKind::Router;
}

void Entity::request_release() {
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
        if (h) h->holder_release(self);
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
