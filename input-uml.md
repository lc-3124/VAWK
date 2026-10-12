# vaterm/input.hpp — UML 类图

> 对象：`tui-utils/include/vaterm/input.hpp`（`namespace vaterm`）
> 说明：`InputData` 的 `InputType / MouseButton / MouseAction / KeyCode / MouseData / KeybdData / Value` 都是**嵌套在 `InputData` 内部**的子类型，图中为便于排版单独列出。

---

## 一、类图（Mermaid）

```mermaid
classDiagram
    direction LR

    namespace vaterm {

        class InputData {
            +InputType inputType
            +Value value
            +InputData()
            +makeMouse(const MouseData& m) InputData$
            +makeKeybd(const KeybdData& k) InputData$
        }

        class Value {
            <<union>>
            +MouseData mouse
            +KeybdData keybd
            +Value()
            +~Value()
        }

        class MouseData {
            +int col
            +int row
            +MouseButton button
            +MouseAction action
            +bool isShift
            +bool isAlt
            +bool isCtrl
        }

        class KeybdData {
            +char32_t cp
            +KeyCode code
            +bool isShift
            +bool isAlt
            +bool isCtrl
        }

        class InputType {
            <<enumeration>>
            MOUSE
            KEYBD
            NON
        }
        class MouseButton {
            <<enumeration>>
            NONE
            LEFT
            MIDDLE
            RIGHT
        }
        class MouseAction {
            <<enumeration>>
            RELEASE
            PRESS
            DRAG
            MOVE
            SCROLL_UP
            SCROLL_DOWN
        }
        class KeyCode {
            <<enumeration>>
            NONE
            UP
            DOWN
            LEFT
            RIGHT
            HOME
            END
            PGUP
            PGDN
            INS
            DEL
            F1
            F12
            ESC
            TAB
            ENTER
            BACKSPACE
        }
        note for KeyCode "F2..F11 略；完整枚举为 F1..F12"

        class input_fsm {
            -State state_
            -string acc_
            +feed(byte b, deque~InputData~& out) void
            +feed(string_view bytes, deque~InputData~& out) void
            +pending() bool
            +flush_pending(deque~InputData~& out) void
            +reset() void
            -push_key(out, cp, code, shift, alt, ctrl) void$
            -step_ground(b, out) bool
            -step_esc(b, out) bool
            -step_csi(b, out) bool
            -step_ss3(b, out) bool
            -step_mouse(b, out) bool
            -step_utf8(b, out) bool
            -parse_csi(params, final, out) void$
            -parse_mouse(seq, out) void$
        }

        class State {
            <<enumeration>>
            GROUND
            ESC
            CSI
            SS3
            MOUSE
            UTF8
        }

        class input {
            -input_fsm fsm_
            -deque~Entry~ buffer_
            -uint64_t seq_
            -mutex m_
            -mutex lifecycle_
            -bool started_
            -condition_variable cv_
            -atomic~bool~ running_
            -atomic~bool~ mouse_enabled_
            -atomic~bool~ keybd_enabled_
            -atomic~InputMode~ state_
            -thread reader_
            +input()
            +~input()
            +start() bool
            +stop() bool
            +enableMouse() bool
            +disableMouse() bool
            +enableKeyBd() bool
            +disableKeyBd() bool
            +getState() InputMode
            +getInput(type) InputData
            +getInput() InputData
            +waitInput(type, timeout_ms) InputData
            +waitInput(timeout_ms) InputData
            -reader_loop() void
            -enqueue(produced) void
            -update_state() void
            -match(type, d) bool$
            -latest(type) InputData
        }

        class Entry {
            +uint64_t seq
            +InputData data
        }

        class InputMode {
            <<enumeration>>
            ON
            MOUSE_DISABLE
            KEYBD_DISABLE
            INPUT_DISABLE
            OFF
        }

        class encode_utf8 {
            <<free function>>
            +encode_utf8(char32_t cp) string
        }
        class KeyPrintf {
            <<free function>>
            +KeyPrintf(KeyCode code) string_view
            +KeyPrintf(const KeybdData& key) string
        }
        class MousePrintf {
            <<free function>>
            +MousePrintf(MouseButton b) string_view
            +MousePrintf(MouseAction a) string_view
            +MousePrintf(const MouseData& m) string
        }
        class InputPrintf {
            <<free function>>
            +InputPrintf(const InputData& e) string
        }
    }

    %% ---- 组合 / 聚合 ----
    InputData *-- Value : value
    Value --> MouseData : mouse
    Value --> KeybdData : keybd
    InputData --> InputType : inputType
    MouseData --> MouseButton : button
    MouseData --> MouseAction : action
    KeybdData --> KeyCode : code

    input *-- input_fsm : fsm_
    input *-- "0..*" Entry : buffer_
    Entry --> InputData : data
    input_fsm *-- State : state_

    %% ---- 依赖 ----
    input_fsm ..> InputData : makeMouse / makeKeybd
    input ..> InputData : 返回
    KeyPrintf ..> InputData : 读取 KeybdData / KeyCode
    KeyPrintf ..> encode_utf8 : 复用
    MousePrintf ..> InputData : 读取 MouseData
    InputPrintf ..> KeyPrintf : 分派
    InputPrintf ..> MousePrintf : 分派
```

---

## 二、数据流

```
                 ┌──────────────────────────── input（模块）────────────────────────────┐
                 │                                                                        │
 stdin(bytes) ──►│  reader_loop()  ──  poll(50ms) / poll(0) 排空  ──►  feed(byte)          │
                 │        ▲                              │                                │
                 │        │ (后台线程 reader_)            ▼                                │
                 │        │                        input_fsm（状态机）                     │
                 │        │                GROUND→ESC→CSI/SS3/MOUSE ；GROUND→UTF8         │
                 │        │                              │                                │
                 │        │                        push_key() / parse_mouse()              │
                 │        │                              ▼                                │
                 │        │                        InputData（键/鼠/空包）                   │
                 │        └──────────────  enqueue() ──► buffer_（环形缓冲, 容量 16）        │
                 │                                             │                           │
                 │             getInput() / waitInput() ◄───────┘                          │
                 └────────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
                     消费者（Entity / 业务）拿 InputData
```

- **生产**：`reader_loop()`（后台线程）→ `input_fsm::feed` → `enqueue()` → `buffer_`
- **消费**：`getInput(type)` 非阻塞取“历史里最新的匹配项”；`waitInput(type,timeout)` 阻塞等“新的匹配项”（`cv_` 通知）
- **取用表（打印）**：`InputPrintf → KeyPrintf / MousePrintf → encode_utf8`

---

## 三、要点

- `InputData::Value` 是 **union**：仅 `inputType` 指定的成员有效（`MOUSE`→`mouse`，`KEYBD`→`keybd`，`NON`→空包）。
- `InputData` 只能通过 `makeMouse / makeKeybd`（或默认构造成空包）产生，避免外部手动组装 union。
- `input_fsm` 是**内部实现**，单独成类只为脱离终端做单元测试；对外只用 `input`。
- `input` 对外接口：`start / stop`、`enableMouse / disableMouse`、`enableKeyBd / disableKeyBd`、`getState`、`getInput / waitInput`；线程安全。
- 环形缓冲容量由宏 `VATERM_INPUT_BUFFER_SIZE`（默认 16）决定，超出丢弃最旧的。
- 循环依赖：`input` 持有 `input_fsm`，`input_fsm` 产出 `InputData`；两者都只依赖 `input.hpp` 内定义。
