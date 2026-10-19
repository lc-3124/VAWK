# VAWK 约定

简短约定，供贡献者与 AI 参照；不追求覆盖所有细节。

## 目录

- `include/` 是**唯一公共头根**（编译时统一 `-Iinclude`），含 `vaterm` / `vatui` / `vawk` 三个模块。
- `src/` 与 `include/` **镜像**：`include/a/b.hpp` 的实现是 `src/a/b.cpp`。
- `demo/` 下**每个一级子目录 = 一个独立演示项目**（各自带 Makefile）。
- `test/` 用例集中存放，**一个 Makefile 管全部**。
- `lib/`、`build/` 是构建产物，不入版本库。

## 命名

- 命名空间：`vaterm` / `vatui` / `vawk`；`core`、`widget` 是**目录**，不是命名空间。
- 类型：`PascalCase`；函数 / 变量：`snake_case`。
- 宏：`UPPER_SNAKE`。
- 文件名：`snake_case.hpp` / `snake_case.cpp`。
- 头卫兵：`UPPER_SNAKE_HPP`。
- 枚举定义集中在 `vaterm/enums.hpp`。

## 头与实现的分界

一句话判据：**函数体含分支 / 循环 / 查表 / syscall → 头出声明、cpp 出定义；纯转发或纯字符串拼装 → 留在头里 inline。**

- 头里只有声明，加上「1~3 行的纯转发」（例如 `fg(Rgb)` → `fg(r,g,b)`）。
- `cursor.hpp` 整体是纯 `\033[..` 拼串，按判据保持 header-only。
- 模板、宏、纯枚举**只能**留在头里（`vawk/core/event.hpp` 属此类）。
- 头必须**显式**声明自己用到的依赖（不要依赖别的头顺带传递，例如 `term.hpp` 直接 `#include "vaterm/enums.hpp"`）。

## 持有协议

`vawk::Holder` 定义"谁在持有实体、何时可以放弃"的统一协议：

- 持有者继承 `Holder`；`acquire()` 在实体小本子上登记，`disown()` 主动放手，
  析构自动注销全部登记。`disown()` 有两个重载：传 `shared_ptr<Entity>` 按实体
  匹配，传 `const Entity*` 按裸指针匹配；两者都只维护持有记录，**不**销毁实体。
- 实体小本子是一张 `{token, kind, owner}` 表，token 单调递增；注销按 token
  线性查找 + swap-pop，持有者只有个位数，不引入哈希。
- 每次注销一笔后，若实体已启用则调用 `is_orphan()` 判定；`enable()` 成功
  后也会在锁外主动判定一次。默认语义是"没有任何 `System` 持有者即孤儿"
  （`Router` / `Parent` / `Business` 都不算保活）。判定为孤儿就走
  `request_disown()`：
  自保 → 锁内取走全部 owner 并清空小本子 → 锁外逐个调 `on_disown()`。
  `request_disown()` 收尾会复位内部的 `releasing_` 标志，因此持有者若没有
  真正放手，后续持有变化仍可再次触发放手请求。
- 取消最后一个订阅会自动放开 Router 持有（`subscribe` 与 `unsubscribe` 对称）；
  `unsubscribe()` 只把"该实体已无任何订阅"记为待放手，锁外再调
  `disown(const Entity*)`，避免在索引锁内回调实体。
- 放手条件可用 `set_release_policy()` 运行期配置，策略参数是
  `std::vector<HolderInfo>`（每条含 `kind` 与持有者实例 `owner`），返回 true
  表示可以释放；留空则用默认 `is_orphan()` 判定。策略在 `mtx_holders_` 内
  调用，不得反向加锁或回调实体。
- `on_disown()` 默认只丢自己那条记录，**不得**回头调 `remove_holder()`，
  否则重入。
- 实体两阶段：构造只存 `upstream` 的 `weak_ptr`（不入册、不起线程）；
  `enable()` 校验 `weak_from_this()` → `on_subscribe()` 订阅 → 失败回滚 →
  成功起线程；`shutdown()` 是**实体自我关停**（停线程 + 注销订阅），幂等，
  置标志、唤醒、join、`detach()`。它不替持有者放手，也不等同于 `disown()`。
- 实体必须由 `shared_ptr` 托管（`shared_from_this()` 是自保与订阅的前提）；
  子实体对父实体只持弱引用。
- 回调（`on_event` / `on_disown`）中不得销毁自身；在 `on_event()` 所在的
  工作线程里析构实体会直接 `std::terminate`（避免 join 自身与随后的 UAF）。
  队列满时生产者阻塞，容量由 `VAWK_ENTITY_QUEUE_SIZE` 控制。

## 注释

统一用**中文**；技术名词与标识符保留英文：ANSI、SGR、UTF-8、RGB、RAII、CSI 等。

三种注释按**用途**分（不是按文件分）：

| 用途 | 写法 |
|---|---|
| 描述**接口 / 声明**（头里的 namespace、类型、函数、宏） | `/** ... */` + 标准 Doxygen |
| 描述**代码在做什么**（`.cpp` 全部、头里 inline 函数的实现体） | `//` 行注释，不带任何 tag |
| **成员 / 变量**行内 | `///< 说明`，**不对齐**，前面一个空格 |

判据：**"接口契约"用 Doxygen，"执行过程"用 `//`**。因此 `.cpp` 里不出现 `/** */`。

- **`/**` 独占一行，正文从下一行开始**：

  ```cpp
  /**
   * @brief 接管输入：开启鼠标追踪、启动后台解析线程。
   * @param  timeout_ms 负数无限等待；>= 0 有限等待。
   * @return 成功返回 true。
   * @note   lc3124(261013): raw mode 需调用方预先开启。
   * @warning 缓冲区耗尽会丢数据。
   * @todo   复盘状态转移是否有遗漏。
   */
  ```

- 常用 tag：`@brief`（必写）/ `@param` / `@return` / `@note` / `@warning` / `@todo`。
  `@param` **不写** `[in]` / `[out]`。
- **署名批注**沿用 `lc3124(YYMMDD):` 原文，放进 `@note`。
- **私有 / 内部**（`input::Entry`、`input_fsm::State`、cpp 匿名命名空间等）**不给 `/** */` 块**，只用 `//` 或 `///<`；头里只保留**对外 API** 的 Doxygen。
- 文件头保持各文件既有形式（不套 `@file`）。

### 分区横线：`//-` 层级累加

用固定短前缀表意，**不画等长横线**（太长的横线不利于手敲）：

```cpp
//- InputData —— 输入数据包          ← 顶层节，1 个连字符

//-- 鼠标负载                        ← 子节，2 个

//--- 坐标字段                       ← 再细，3 个
```

- 连字符数 = **嵌套绝对深度**，逐层加一，不在父节下重置。
- 只写"起始行"，**不画收尾线**；下一个 `//-` 出现即为上一节结束。
- 标题随意写、美观即可。
- 旧的 `// ====`、`// ──`、`// ----` 一律不采用。

## 构建

- **只有顶层 `Makefile`** 不写具体文件名、全自动发现；项目级 Makefile 可以点名文件。
- 库产物：`libvawk.a` + `libvawk.so`（对象一律 `-fPIC`）。
- demo / test 默认链接静态库 `.a`。

## 编译失败策略

- `src/`（库）失败即**整体失败**。
- `demo/`、`test/` 单个文件失败则**跳过**，并在末尾明确打印，不中断整体。

## 许可

- **GPL-3.0**：强 copyleft，下游链接 / 分发即需开源。
