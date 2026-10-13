#ifndef VAWK_INPUT_HPP
#define VAWK_INPUT_HPP

// -----------------------------------------------------------------------
//  vawk::Input — 终端输入处理器（占位 / 桩）
//
//  本模块用于把 vaterm 的低层终端 I/O
//  （mouse::capture、terminal::read_byte 等）桥接到 vawk 事件
//  系统中。
//
//  目前已定义：
//    - MouseClickEvent  — 鼠标点击/拖动事件的事件类型
//
//  尚待实现：
//    - 键盘事件（按键按下 / 释放）
//    - 终端大小变化事件
//    - 真正读取输入并喂给 Entity 订阅者的循环
//
//  下面的 Input 类目前是桩实现；start() 与 stop() 暂时什么都不做。
// -----------------------------------------------------------------------

#include "event_upstream.hpp"

#include <optional>

namespace vawk {

// 前向声明的事件类型。

// 鼠标按键按下、释放或拖动时发出。
VAWK_EVENT_DEFINE(MouseClickEvent)
    float x, y;                   // 归一化坐标或单元格坐标
    std::optional<int> duration;  // 距上次按下的时间（毫秒），nullopt = 未知
VAWK_EVENT_DEFINE_END

// 输入处理器。
// 这是一个占位实现——尚未完成。
class Input {
  public:
    Input() = default;
    ~Input() = default;

    // 开始监听输入事件。
    void start();

    // 停止监听输入事件。
    void stop();

  private:
    bool running_ = false;
};

}  // namespace vawk

#endif  // VAWK_INPUT_HPP
