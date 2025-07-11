## Still under developing .......

#  VAGUI

### To do zn_cn
0. [ ] 重读源码取证，紧急解决问题
1. [ ] 将isevent 和 getif 方法封装到EventBase中
2. [ ] 抽象出事件上游类，实现更自由高性能的事件机制。
3. [x] 最重要！ 期末考来了，复习备考！ * 正常发挥 *

## suggestions producted by deepseek
以下是VAGUI事件系统现存问题及解决方案的总结表格，便于您写入README.md和后续修复：

### VAGUI事件系统优化路线表
| **问题分类**       | **具体问题描述**                                                                 | **严重等级** | **解决建议**                                                                                                | **实现难度** | **GitHub提交建议**                     |
|--------------------|----------------------------------------------------------------------------------|--------------|------------------------------------------------------------------------------------------------------------|--------------|-----------------------------------------|
| **生命周期管理**   | 使用裸指针`VaEntity*`存储实体，实体销毁后导致未定义行为                                          | ⭐⭐⭐⭐⭐        | 改用`std::weak_ptr<VaEntity>`，分发前用`lock()`检查实体存活 []                      | 中等         | `VaEventLoop.hpp/cpp` 重构监听者存储结构 |
| **事件分发延迟**   | `thr_DispatchLoop`固定10ms轮询休眠，高并发下延迟显著                                           | ⭐⭐⭐⭐         | 引入`std::condition_variable`，事件入队时`notify_one()`唤醒线程 []                  | 简单         | `VaEventLoop.cpp` 线程调度逻辑           |
| **内存效率**       | `Listeners`使用`vector<vector>`存储稀疏事件ID，内存利用率低                                    | ⭐⭐           | 事件ID密集时保留当前结构；稀疏时改用`unordered_map<size_t, vector>` []                         | 低           | `VaEventLoop.hpp` 数据结构优化           |
| **并发安全**       | `DispatchOnce`遍历监听者时不加锁，与`UnRegister`并发操作导致迭代器失效                          | ⭐⭐⭐⭐         | 分发前复制事件ID对应的监听者列表：`auto local = Listeners[eid]` []                               | 中等         | `VaEventLoop.cpp` 分发逻辑重构           |
| **架构扩展性**     | 全局静态实例`va_event_loop`阻碍多事件循环支持                                                | ⭐⭐⭐          | 改用依赖注入，通过构造函数传递`EventLoop`实例 []                                              | 中等         | 新增`VaContext`类管理事件循环实例       |
| **代码可维护性**   | `VA_EVENT_DEFINE`宏隐藏实现细节，调试困难                                                    | ⭐⭐           | CRTP模式替代宏：<br>`template<T> struct EventTraits;`<br>`struct MyEvent : EventBase, EventTraits<MyEvent>` | 高           | `VaEvent.hpp` 事件定义机制重构           |
| **注释误导**       | `VaEntity::eventPush`注释称"立即处理事件"，实际仅缓冲                                         | ⭐            | 修正注释：<br>"缓冲事件至队列，需调用processOneEvent处理"                                                  | 低           | `VaEntity.hpp` 文档更新                 |
| **直发机制风险**   | `SendEventToEntityDirect()`未检查实体存活，且实体内部队列用互斥锁影响实时性                    | ⭐⭐⭐⭐         | 1. 直发前用`weak_ptr`检查实体<br>2. 实体内部队列改用无锁结构（如`moodycamel::ConcurrentQueue`） [] | 高           | `VaEntity.hpp/cpp` 队列与直发逻辑优化   |

### 补充说明
1. **优先级建议**  
   **立即修复**：生命周期管理 ▶ 直发机制安全 ▶ 事件分发延迟  
   **阶段迭代**：内存效率优化 ▶ 架构扩展性 ▶ 宏替换
   
3. **README.md 更新要点**  
   ```markdown
   ## 待优化事项
   - [ ] **高优先级**：实体生命周期安全（弱指针改造）  
   - [ ] **高优先级**：直发通道线程安全（无锁队列集成）  
   - [ ] **中优先级**：事件循环唤醒机制（条件变量替代轮询）  
   - [ ] **低优先级**：事件定义宏替换（CRTP模式）
   ```
