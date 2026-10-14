#ifndef VATERM_CURSOR_HPP
#define VATERM_CURSOR_HPP

//- vaterm::cursor — 光标操作的 ANSI 转义序列
//
// 所有函数都返回包含原始转义序列的 std::string。
// 它们无状态——不缓存任何终端状态。
//
// 移动：
//   move_to(row, col)       — 绝对定位（1 起）
//   move(dir, steps)        — 相对移动（UP/DOWN/LEFT/RIGHT）
//
// 保存 / 恢复：
//   save() / restore()      — DEC 私有模式 7（SCP / RCP）
//
// 可见性与形状：
//   show() / hide()         — DECTCEM (?25h / ?25l)
//   shape(CursorShape)      — DECSCUSR（1..6，见 enums.hpp）
//
// 查询：
//   report_position()       — CPR (\033[6n)；终端在 stdin 上回应

#include "enums.hpp"

#include <format>
#include <string>

namespace vaterm {
namespace cursor {

//- 移动

/**
 * @brief 把光标移动到绝对 (row, col) 位置（1 起，1 = 左上角）。
 * @param row 目标行，从 1 开始。
 * @param col 目标列，从 1 开始。
 * @return CUP 转义序列。
 */
inline std::string move_to(uint16_t row, uint16_t col) {
    return std::format("\033[{};{}H", row, col);
}

/**
 * @brief 相对当前位置移动光标。
 * @param dir   CursorDir::UP、DOWN、LEFT 或 RIGHT。
 * @param steps 移动的格子数。
 * @return CUU/CUD/CUF/CUB 转义序列。
 */
inline std::string move(CursorDir dir, uint16_t steps) {
    constexpr const char* seqs[] = {"A", "B", "C", "D"};
    return std::format("\033[{}{}", steps, seqs[static_cast<uint8_t>(dir)]);
}

//- 保存 / 恢复

/**
 * @brief 保存当前光标位置（DEC 私有模式 7，SCO 序列）。
 * @note 用 restore() 恢复。
 * @return 保存序列。
 */
inline std::string save() {
    return "\0337";
}

/**
 * @brief 恢复最近一次 save() 保存的光标位置。
 * @return 恢复序列。
 */
inline std::string restore() {
    return "\0338";
}

//- 可见性与形状

/**
 * @brief 让光标可见（DECTCEM）。
 * @return 显示序列。
 */
inline std::string show() {
    return "\033[?25h";
}

/**
 * @brief 隐藏光标（DECTCEM）。
 * @return 隐藏序列。
 */
inline std::string hide() {
    return "\033[?25l";
}

/**
 * @brief 改变光标形状（DECSCUSR）。
 * @note xterm 及大多数现代终端模拟器都支持。可用形状
 *        （闪烁/常亮 方块、下划线、竖条）见 enums.hpp 中的 CursorShape。
 * @param s 目标形状。
 * @return DECSCUSR 转义序列。
 */
inline std::string shape(CursorShape s) {
    return std::format("\033[{} q", static_cast<uint8_t>(s));
}

//- 查询

/**
 * @brief 请求终端上报光标位置（CPR）。
 * @note 终端会在 stdin 上以 "\033[row;colR" 回应；
 *        用 terminal::read_byte() 读取响应。
 * @return CPR 查询序列。
 */
inline std::string report_position() {
    return "\033[6n";
}

} // namespace cursor
} // namespace vaterm

#endif // VATERM_CURSOR_HPP
