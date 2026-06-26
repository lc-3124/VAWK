# vawk — 事件系统实现思路

> vawk 是构建在 vatui 之上的事件驱动 TUI 框架。
> 本文档详细说明事件系统的内部设计、数据结构、线程模型以及设计取舍。

---

## 1. 架构总览

```
  用户代码 (定义事件类型 / 订阅 / push / 业务逻辑)
       │
       ▼
  ┌─────────────────────────────────────────────┐
  │               EventRouter                    │  事件总线
  │  ┌──────────────┐  ┌─────────────────────┐  │
  │  │ event_buffer_ │  │  listeners_[]        │  │  按 event_id 索引
  │  └──────┬───────┘  └─────────────────────┘  │
  └─────────┼───────────────────────────────────┘
            │ dispatch_once()
            ▼
  ┌─────────────────────────────────────────────┐
  │               Entity                          │  事件接收方
  │  ┌────────────────┐  ┌──────────────────┐  │
  │  │ event_buffer_   │  │ on_event()       │  │  纯虚，子类实现
  │  └────────────────┘  └──────────────────┘  │
  └─────────────────────────────────────────────┘
```

事件系统分为两层：

1. **EventUpstream（事件源）** — 维护监听者注册表，负责把事件投递给所有相关 Entity
2. **Entity（事件接收方）** — 自带的线程安全缓冲队列，子类重写 `on_event()` 处理事件

```cpp
// 事件流转路径
push(evt) → event_buffer_ → dispatch_once()
                                  ↓
                            listeners_[evt->id()]
                                  ↓
                     entity->push_event(evt)     ← 线程安全
                                  ↓
                            entity 的缓冲队列
                                  ↓
                            process_one_event()
                                  ↓
                            on_event(evt)         ← 用户业务逻辑
```

---

## 2. 事件类型系统

### 2.1 设计目标

- 事件类型在编译时定义，运行时按类型派发
- 不依赖 C++ RTTI（`type_info` / `dynamic_cast`）
- 每个事件类型在进程内有唯一 ID
- 事件类型定义语句尽可能简洁

### 2.2 类型 ID 生成

```cpp
// 全局 atomic 计数器
inline std::atomic<size_t>& global_event_id_counter() {
    static std::atomic<size_t> counter{0};
    return counter;
}

// 每个 T 实例化时获得一个单调递增的 ID
template <typename T>
size_t event_type_id() {
    static size_t id = global_event_id_counter()++;
    return id;
}
```

原理：

- `event_type_id<T>()` 是函数模板，每个 `T` 对应一个实例化
- 函数内部 `static size_t id` 在首次调用时初始化（C++11 起线程安全）
- 初始化时从全局 atomic 计数器取下一个值
- 每个 `T` 的 ID 在进程内唯一、一旦分配不再改变

对比 C++ RTTI 方案：

| 方案 | 依赖 | 运行时开销 | 跨编译单元稳定性 |
|------|------|-----------|-----------------|
| `std::type_info::hash_code()` | `type_info` | 有 | 跨模块不一致 |
| 自己造 type_id | 无 | 一次 atomic load | 进程内唯一 |
| `event_type_id<T>()` | 无 | 一次 static var 检查 | 进程内唯一 |

### 2.3 事件基类与宏

```cpp
namespace event {

struct Base {
    virtual ~Base() = default;
    virtual size_t id() const = 0;
};

} // namespace event
```

`VAWK_EVENT_DEFINE(Name)` 宏展开为：

```cpp
struct Name : public vawk::event::Base {
    size_t id() const override {
        return event_type_id<Name>();
    }
    // ← 用户在这里插入字段
};
```

`VAWK_EVENT_DEFINE_END` 闭合。

宏的存在是为了让定义事件的代码量降到最低，且保证 `id()` 覆盖不被遗漏。

### 2.4 运行时类型识别

```cpp
template <typename T>
bool event::is(const Base& e) {
    return e.id() == event_type_id<T>();
}

template <typename T>
const T* event::cast(const Base& e) {
    if (is<T>(e))
        return &static_cast<const T&>(e);
    return nullptr;
}
```

`is<T>()` / `cast<T>()` 完全基于 type_id 比较。因为 `id()` 是虚函数，
`Base&` 能调用到最派生类的 ID。没有 RTTI 开销。

---

## 3. EventUpstream — 事件源

### 3.1 职责

- 维护一组订阅关系（谁对什么事件感兴趣）
- 提供线程安全的注册/注销接口
- 定义派发接口，让子类决定"事件从哪里来"

### 3.2 双重索引

```cpp
// listeners_[event_id] = 该事件类型的所有订阅者
std::vector<std::vector<Entity*>> listeners_;

// entity_events_[entity] = 该实体订阅了哪些事件类型
std::unordered_map<Entity*, std::vector<size_t>> entity_events_;
```

两个方向都索引，保证两种高频操作都快：

| 操作 | 索引 | 复杂度 |
|------|------|--------|
| 派发：按 event_id 找所有 Entity | `listeners_` | O(1) 查表 + O(N) 遍历 |
| 注销：给 Entity 找出它订阅的所有类型 | `entity_events_` | O(1) 查表 |

如果不维护反向索引，`unregister_listener(Entity*)` 就需要遍历每个
`listeners_[i]` 逐一查找该 Entity，复杂度 O(事件类型数 × 每类型订阅数)。

### 3.3 线程模型

```cpp
protected:
    std::mutex mutex_;                    // 保护 listeners_ / entity_events_
    std::mutex cv_mutex_;                 // 保护 running_ + CV 等待
    std::condition_variable cv_;          // 通知后台线程
    std::atomic<bool> running_{false};    // 线程生命周期标志
    std::thread event_thread_;            // 后台派发线程
```

`start_event_loop()` / `stop_event_loop()` 管理后台线程：

- `start_event_loop`: 以 `running_ = true` 启动线程跑 `dispatch_loop()`
- `stop_event_loop`: 置 `running_ = false`、通知 CV、join 线程

后台线程不是必须的。用户可以手动调用 `dispatch_once()` 实现单线程主循环。

### 3.4 为什么 Entity 存原始指针

原始指针（而非 `shared_ptr` / `weak_ptr`）是刻意选择：

- 避免引用循环：Entity 持有 upstream 指针，upstream 持有 Entity 指针
- `shared_from_this()` 需要 `std::enable_shared_from_this`，引入额外约束
- Entity 通常由应用拥有，生命周期明确

约定：

> 调用者必须确保 Entity 在它注册的 EventUpstream 之前销毁。
> Entity 析构时自动从所有 upstream 取消注册（通过 `entity_events_` 反向索引）。

---

## 4. Entity — 事件接收方

### 4.1 双缓冲设计

每个 Entity 拥有独立的线程安全缓冲：

```cpp
std::mutex mutex_;
std::queue<std::shared_ptr<event::Base>> event_buffer_;
```

`EventUpstream::dispatch_once()` 向每个 Entity 调用 `push_event()`，
其实只是锁住 Entity 的 mutex 推入队列——不阻塞总线。

Entity 的拥有者在自己的上下文中调用 `process_one_event()`，
取出事件后调用虚函数 `on_event()`，锁在取出后释放，不会在业务逻辑中持有锁。

### 4.2 ProcessResult

`process_one_event()` 返回三种状态：

| 返回值 | 含义 |
|--------|------|
| `Processed` | 事件已出队并交付 `on_event()` |
| `Discarded` | 事件为空指针，已丢弃 |
| `Empty` | 缓冲为空，无事可做 |

### 4.3 订阅管理

```cpp
void subscribe(EventUpstream* upstream, std::vector<size_t> event_ids);
void unsubscribe(EventUpstream* upstream, std::vector<size_t> event_ids);
void unsubscribe(EventUpstream* upstream);
```

`subscribe()` 调用上游的 `register_listener()` 逐一注册，
并记录上游指针到 `upstreams_`（去重），确保析构时能自动注销。

---

## 5. EventRouter — 具体事件总线

### 5.1 职责

`EventRouter` 是 `EventUpstream` 的唯一具体实现：

```cpp
class EventRouter : public EventUpstream {
    std::queue<std::shared_ptr<event::Base>> event_buffer_;
};
```

- `push(evt)` — 入队 + 通知 CV
- `dispatch_once()` — 出队一个事件，查找 `listeners_[eid]`，逐个 `push_event()`
- `dispatch_loop()` — 循环调用 `dispatch_once()`，配合后台线程

### 5.2 派发流程

```
push(shared_ptr<Base> evt)
  │
  ├─ mutex_.lock()
  ├─ event_buffer_.push(evt)
  ├─ cv_.notify_one()
  └─ mutex_.unlock()
          │
          ▼  (下一次 dispatch_once 调用)
  mutex_.lock()
  evt = event_buffer_.front() + pop()
  mutex_.unlock()
          │
          ▼
  eid = evt->id()
  mutex_.lock()
  targets = listeners_[eid]    // 拷贝一份
  mutex_.unlock()
          │
          ▼
  for each entity in targets:
      entity->push_event(evt)   // 线程安全入队
```

关键设计点：

- `dispatch_once` 在锁外调用 `entity->push_event()`，避免死锁
- 监听者列表在锁内拷贝一份，锁外遍历，允许派发过程中修改注册表
- `push()` 只锁 `mutex_`（event buffer），不锁 `cv_mutex_`

### 5.3 后台线程 dispatch_loop

```cpp
void EventRouter::dispatch_loop() {
    while (true) {
        if (!running_ && event_buffer_.empty())
            break;
        dispatch_once();
        std::this_thread::yield();
    }
}
```

设计考量：

- 手动 `dispatch_once()` + 单线程主循环是最常用的场景
- 后台线程循环以 `running_` + buffer empty 为终止条件
- `yield()` 避免空闲时忙等；生产-消费仅在 push 侧通知 CV
- 典型 TUI 应用仅在需要后台定时器或异步 I/O 时使用后台线程

---

## 6. 设计取舍

### 6.1 为什么不直接用 shared_ptr 管理 Entity

早期设计尝试过 `shared_from_this` + 注册 `weak_ptr<Entity>`。

问题：

- `Entity` 需要继承 `std::enable_shared_from_this`
- `entity_events_` 转换成 `std::unordered_map<weak_ptr, ...>`，查找需 lock()
- 应用层最自然的用法是栈上 Entity 或 unique_ptr，而非 shared_ptr

当前设计：原始指针 + 约定。代价是调用者必须保证生命周期。

### 6.2 为什么 EventUpstream 是抽象基类

不只为了 `EventRouter` 一个子类。预留的场景：

- 直接从 `vawk::Input` 继承（不再需要中间 buffer，将输入流直接投递）
- 从 socket / pipe 读取事件的 AsyncEventSource
- 定时器事件源

`dispatch_once()` / `dispatch_loop()` 抽象接口让每种事件源可以有自己的
事件生产逻辑，共享同一套订阅管理机制。

### 6.3 为什么不用中央 EventBus 单例

`EventRouter` 设计为可创建多个实例，而不是全局单例。

原因：

- 大型应用可能需要多个独立的事件域（如每个窗口有自己的总线）
- 测试时可以创建独立的 Router，不需要全局状态
- 用户可以选择手动维护单例，框架不做强制

---

## 7. 文件组织

| 文件 | 内容 | 编译 |
|------|------|------|
| `include/vawk/event.hpp` | 事件基类、类型 ID 生成、is/cast、宏 | header-only |
| `include/vawk/entity.hpp` | Entity 声明 | header-only |
| `include/vawk/event_upstream.hpp` | EventUpstream 声明 | header-only |
| `include/vawk/event_router.hpp` | EventRouter 声明 | header-only |
| `include/vawk/input.hpp` | MouseClickEvent、Input 骨架 | header-only |
| `src/vawk/entity.cpp` | Entity 实现：缓冲、订阅、析构 | 编译为 entity.o |
| `src/vawk/event_upstream.cpp` | EventUpstream 实现：注册表、线程 | 编译为 event_upstream.o |
| `src/vawk/event_router.cpp` | EventRouter 实现：push、dispatch | 编译为 event_router.o |
| `doc/dev/vawk.md` | 本文档 | — |
| `doc/man/vawk.md` | 用户手册 | — |

### 链接

```bash
make lib  # 编译所有 .o
# 或手动：
g++ --std=c++23 -Iinclude -Itui-utils/include \
    -c src/vawk/entity.cpp -o entity.o
g++ --std=c++23 -Iinclude -Itui-utils/include \
    -c src/vawk/event_upstream.cpp -o event_upstream.o
g++ --std=c++23 -Iinclude -Itui-utils/include \
    -c src/vawk/event_router.cpp -o event_router.o
```
