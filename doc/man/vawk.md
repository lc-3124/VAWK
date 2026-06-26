# vawk 用户手册

> vawk 是构建在 vatui 之上的事件驱动 TUI 框架。
> 提供类型安全的事件定义、灵活的订阅/派发机制，
> 以及线程安全的实体模型。

---

## 核心概念

### 事件（Event）

应用中的"发生了某件事"。比如鼠标点击、键盘按下、定时器到期。
每个事件是一个继承自 `vawk::event::Base` 的结构体，携带自定义数据。

### 实体（Entity）

"谁关心这件事"。Entity 是事件接收方，继承 `vawk::Entity`，
重写 `on_event()` 方法来处理感兴趣的事件。

### 事件总线（EventRouter）

"谁负责送信"。EventRouter 管理订阅关系，接收 `push()` 的事件，
分发到订阅了该事件类型的所有 Entity。

### 工作流

```
定义事件类型 → 创建 Entity → 重写 on_event() → 订阅 → push 事件 → 派发
```

---

## 快速开始

```cpp
#include <vawk.hpp>
#include <cstdio>

using namespace vawk;

// 1. 定义事件类型
VAWK_EVENT_DEFINE(MyEvent)
    int value;
VAWK_EVENT_DEFINE_END

// 2. 创建实体
class MyEntity : public Entity {
  protected:
    void on_event(std::shared_ptr<event::Base> evt) override {
        if (auto* e = event::cast<MyEvent>(*evt)) {
            std::printf("Got value: %d\n", e->value);
        }
    }
};

int main() {
    // 3. 创建事件总线
    EventRouter router;

    // 4. 创建实体并订阅
    MyEntity entity;
    entity.subscribe(&router, {event_type_id<MyEvent>()});

    // 5. 推送事件
    auto evt = std::make_shared<MyEvent>();
    evt->value = 42;
    router.push(evt);

    // 6. 派发
    router.dispatch_once();

    // 7. 消费（Entity 内部）
    entity.process_one_event();  // → 输出 "Got value: 42"
}
```

编译：

```bash
make lib
g++ --std=c++23 -Iinclude -Itui-utils/include \
    myapp.cpp \
    build/bin.o/entity.o build/bin.o/event_upstream.o \
    build/bin.o/event_router.o build/bin.o/vatui.o \
    -o myapp
```

---

## 定义事件

### 宏语法

```cpp
VAWK_EVENT_DEFINE(EventName)
    // 字段定义
VAWK_EVENT_DEFINE_END
```

展开后等价于：

```cpp
struct EventName : public vawk::event::Base {
    size_t id() const override {
        return event_type_id<EventName>();
    }
    // 字段
};
```

### 已完成的事件类型

目前在 `include/vawk/input.hpp` 中定义了鼠标事件：

```cpp
VAWK_EVENT_DEFINE(MouseClickEvent)
    float x, y;                   // 坐标（标准化或单元格坐标）
    std::optional<int> duration;  // 距离上次按下的毫秒数
VAWK_EVENT_DEFINE_END
```

### 运行时判断类型

```cpp
// 检查事件是否为指定类型
if (event::is<MyEvent>(*evt)) { ... }

// 安全向下转型（非匹配类型返回 nullptr）
if (auto* e = event::cast<MyEvent>(*evt)) {
    process(e->value);
}
```

---

## Entity — 事件接收方

### 基本用法

继承 `Entity` 并重写 `on_event()`：

```cpp
class MyHandler : public Entity {
  protected:
    void on_event(std::shared_ptr<event::Base> evt) override {
        // 用 event::is / event::cast 判断类型
        if (auto* m = event::cast<MouseClickEvent>(*evt)) {
            handle_click(m->x, m->y);
        }
    }
};
```

### 缓冲控制

```cpp
void push_event(std::shared_ptr<event::Base> evt);
// 线程安全入队。由 EventUpstream 调用，通常不需要手动调用。

Entity::ProcessResult process_one_event();
// 从内部缓冲取一个事件并调用 on_event()。
// 返回 Processed / Discarded / Empty。
```

### 订阅

```cpp
// 订阅多个事件类型
entity.subscribe(&router, {event_type_id<EventA>(), event_type_id<EventB>()});

// 取消订阅（部分）
entity.unsubscribe(&router, {event_type_id<EventA>()});

// 取消订阅（全部）
entity.unsubscribe(&router);
```

### 标签

为 Entity 设置可读标签，方便调试：

```cpp
entity.set_label("input_handler");
std::string name = entity.label();
```

---

## EventRouter — 事件总线

### 推送事件

```cpp
EventRouter router;

auto evt = std::make_shared<MyEvent>();
evt->value = 100;
router.push(evt);
```

`push()` 是线程安全的：将事件放入内部队列，唤醒等待中的派发线程。

### 手动派发

```cpp
router.dispatch_once();
```

从队列取出**一个**事件，分发给所有订阅了该类型的 Entity。
典型用法是在主循环中每帧调用一次或数次。

### 后台派发

```cpp
// 在单独线程中运行 dispatch_loop()
router.start_event_loop();

// ... 其他线程可以 push ...

// 停止后台线程
router.stop_event_loop();
```

---

## 完整示例 — 鼠标画板

```cpp
#include <vawk.hpp>
#include <vatui.hpp>
#include <cstdio>

using namespace vawk;
using namespace vatui;

// 自定义事件
VAWK_EVENT_DEFINE(QuitEvent)
VAWK_EVENT_DEFINE_END

VAWK_EVENT_DEFINE(PaintEvent)
    int col, row;
    std::string color_sgr;
VAWK_EVENT_DEFINE_END

// 输入桥接：将 vatui 输入转为 vawk 事件
class InputBridge {
  public:
    InputBridge(EventRouter& router) : router_(router) {}

    void poll() {
        auto inp = VaTui::instance().waitInput();
        if (inp.type == InputType::KEY && inp.key.cp == 'q')
            router_.push(std::make_shared<QuitEvent>());
        if (inp.type == InputType::MOUSE) {
            auto& m = inp.mouse;
            auto evt = std::make_shared<PaintEvent>();
            evt->col = m.col;
            evt->row = m.row;
            evt->color_sgr = (m.button == MouseState::Button::LEFT
                ? "\033[44m" : "\033[41m");
            router_.push(evt);
        }
    }

  private:
    EventRouter& router_;
};

// 画板 Entity
class Canvas : public Entity {
  public:
    Canvas(Framebuffer& fb) : fb_(fb) {}

  protected:
    void on_event(std::shared_ptr<event::Base> evt) override {
        if (auto* p = event::cast<PaintEvent>(*evt)) {
            auto style = Style{.fg_sgr = "\033[37m", .bg_sgr = p->color_sgr};
            fb_.fillRegion({p->col, p->row, 1, 1, ' ', style});
        }
    }

  private:
    Framebuffer& fb_;
};

int main() {
    auto& tui = VaTui::instance();
    tui.init();
    tui.enableMouse();
    auto& fb = tui.buffer();

    EventRouter router;
    Canvas canvas(fb);
    InputBridge bridge(router);

    canvas.subscribe(&router, {event_type_id<PaintEvent>()});

    bool quit = false;
    QuitEvent q;
    while (!quit) {
        bridge.poll();           // 读取输入 → push 事件
        router.dispatch_once();  // 派发一个事件
        canvas.process_one_event();  // 实体处理
        fb.swap();               // 渲染
    }
}
```

---

## API 速查

### 事件类型

```cpp
// 头文件
#include <vawk/event.hpp>

// 宏
VAWK_EVENT_DEFINE(Name) ... VAWK_EVENT_DEFINE_END

// 类型 ID
template <typename T>
size_t event_type_id<T>();

// 运行时识别
template <typename T>
bool event::is(const event::Base& e);

template <typename T>
const T* event::cast(const event::Base& e);
```

### Entity

```cpp
// 头文件
#include <vawk/entity.hpp>

class MyEntity : public Entity {
  protected:
    void on_event(std::shared_ptr<event::Base> evt) override;
};

// 方法
void push_event(std::shared_ptr<event::Base> evt);
Entity::ProcessResult process_one_event();

void subscribe(EventUpstream* upstream, std::vector<size_t> event_ids);
void unsubscribe(EventUpstream* upstream, std::vector<size_t> event_ids);
void unsubscribe(EventUpstream* upstream);

std::string label() const;
void set_label(std::string l);
```

`Entity::ProcessResult` 取值：

| 值 | 含义 |
|----|------|
| `Processed` | 事件已处理 |
| `Discarded` | 空事件，已丢弃 |
| `Empty` | 缓冲为空 |

### EventRouter

```cpp
// 头文件
#include <vawk/event_router.hpp>

EventRouter router;

bool push(std::shared_ptr<event::Base> evt);
void dispatch_once();

// 后台线程
void start_event_loop();
void stop_event_loop();
bool is_running() const;
```

---

## 构建

```bash
make lib
```

链接时需要 vawk 各模块的 `.o`：

```bash
g++ --std=c++23 -Iinclude -Itui-utils/include \
    myapp.cpp \
    build/bin.o/entity.o \
    build/bin.o/event_upstream.o \
    build/bin.o/event_router.o \
    build/bin.o/vatui.o \
    -o myapp
```

---

## 注意事项

### 生命周期

Entity 必须在它注册的 EventUpstream 之前销毁。
Entity 析构时会自动从所有 upstream 取消注册。

```cpp
EventRouter router;
MyEntity entity;
entity.subscribe(&router, {event_type_id<MyEvent>()});
// entity 先析构 → 自动 cancel 注册
// router 后析构
```

### 线程安全

- `push()` 是线程安全的，可从任意线程调用
- `dispatch_once()` 每次独立加锁，解锁后调用 `Entity::push_event()`
- Entity 的 `push_event()` / `process_one_event()` 是线程安全的
- **不建议**在 `on_event()` 中直接操作 Framebuffer（非线程安全）

### 派发与消费

`router.dispatch_once()` 只把事件放入 Entity 的缓冲队列。
Entity 需要主动调用 `process_one_event()` 触发 `on_event()`。

```cpp
// 典型主循环
while (running) {
    bridge.poll();              // 读取输入 → push
    router.dispatch_once();     // 总线 → Entity 队列
    entity.process_one_event(); // Entity → on_event()
    fb.swap();                  // 渲染
}
```

如果使用后台线程 `start_event_loop()`，dispach 在后台自动执行，
Entity 的 `process_one_event()` 仍需在主线程或其他线程调用。
