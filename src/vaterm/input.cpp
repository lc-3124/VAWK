//- vaterm::input — 实现
//
//  input 是 vaterm 里唯一带状态、带后台线程的模块，因此单独编译成 .cpp
//  （其余模块仍是 header-only）。声明见 include/vaterm/input.hpp。

#include "vaterm/input.hpp"
#include "vaterm/term.hpp"

// 标准库（显式列出本实现直接使用的头，避免依赖传递包含）
#include <chrono>
#include <deque>
#include <string>
#include <string_view>
#include <utility>

// Linux 平台
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace vaterm {

//- InputData 实现

// 默认构造：inputType 的类内初值就是 NON，联合体负载不用碰，于是得到一个“空包”。
InputData::InputData() {}

// Value 是联合体，两个成员（MouseData / KeybdData）都带默认成员初值，
// 属于“非平凡”类型，因此联合体必须显式给出构造与析构；此处二者都不需要额外动作。
InputData::Value::Value() {}

InputData::Value::~Value() {}

// 构造鼠标包：先把标签 inputType 置为 MOUSE，再写入联合体里的 mouse 负载。
InputData InputData::makeMouse(const MouseData& m) {
    InputData d;
    d.inputType   = InputType::MOUSE;
    d.value.mouse = m;
    return d;
}

// 构造键盘包：与 makeMouse 完全对称，只是标签换成 KEYBD、负载换成 keybd。
InputData InputData::makeKeybd(const KeybdData& k) {
    InputData d;
    d.inputType   = InputType::KEYBD;
    d.value.keybd = k;
    return d;
}

 // vaterm::encode_utf8(...)
 //
 // 参数：
 // char32_t cp ， 用于接受码点
 //
 // 返回:
 // std::string , 换好的字符串，wstring写起来让人火大，我专门弄了一个utf库用string处理宽字符。
 //
 // lc3124(260923):把 Unicode 码点编码成 UTF-8 字符串供 KeyPrintf 复用；虽然和vaterm::utf模块
 // 的职责交叉，但是这个模块是输入引擎的必要功能，我不打算让这个模块依赖于vaterm::utf模块。
std::string encode_utf8(char32_t cp) {
    std::string s;

    // 这里放一张表，参考一下utf的编码方式，我们比较首字节大小来判断编码模式，然后把
    // 编码位取出来，合成一个字符
    //   0x00    .. 0x7F     → 1 0xxxxxxx
    //   0x80    .. 0x7FF    → 2 110xxxxx 10xxxxxx
    //   0x800   .. 0xFFFF   → 3 1110xxxx 10xxxxxx 10xxxxxx
    //   0x10000 .. 0x10FFFF → 4 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

    if (cp < 0x80) {
        // ASCII：单字节
        s += static_cast<char>(cp);
    } else if (cp < 0x800) {
        // 2 字节：高 5 位写入首字节，低 6 位写入续字节，移位收集编码位
        // 第一行代码：丢弃7位，拿11000000(0xC0)做OR运算，就能把编码位滤出来，剩下的同理
        s += static_cast<char>(0xC0 | (cp >> 6)); 
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        // 3 字节：CJK
        s += static_cast<char>(0xE0 | (cp >> 12));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        // 4 字节：emoji、生僻字
        s += static_cast<char>(0xF0 | (cp >> 18));
        s += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return s;
}

 // vaterm::KeyPrintf(const InputData::KeybdData& key)
 //
 // 完整键（含普通键）→ 显示串。
 //
 // 参数：
 // const InputData::KeybdData& key ， 键盘负载
 //
 // 返回:
 // std::string , 无修饰的普通键返回字符本身；
 //               其余返回 KeyPrintf(code)解析出来的字面
std::string KeyPrintf(const InputData::KeybdData& key) {
    const bool special = key.code != InputData::KeyCode::NONE;

    if (!special && !key.isCtrl && !key.isAlt && !key.isShift)
        return encode_utf8(key.cp);

    std::string name = special ? std::string(KeyPrintf(key.code)) : encode_utf8(key.cp);

    std::string mods;
    if (key.isCtrl)  mods += "Ctrl+";
    if (key.isAlt)   mods += "Alt+";
    if (key.isShift) mods += "Shift+";
    if (mods.empty()) return name;                          // 无修饰的特殊键：<F11>

    if (special) { name.insert(1, mods); return name; }     // <Ctrl+F11>
    return "<" + mods + name + ">";                         // <Ctrl+a>
}

 // vaterm::MousePrintf(const InputData::MouseData& m)
 //
 // 完整鼠标事件 → 显示串。
 //
 // 参数：
 // const InputData::MouseData& m ， 鼠标负载
 //
 // 返回:
 // std::string , 形如 "<MOUSE (5,7) LEFT PRESS Ctrl>"
std::string MousePrintf(const InputData::MouseData& m) {
    std::string s = "<MOUSE (";
    s += std::to_string(m.col);
    s += ",";
    s += std::to_string(m.row);
    s += ")";
    if (m.button != InputData::MouseButton::NONE) {
        s += " ";
        s += MousePrintf(m.button);
    }
    s += " ";
    s += MousePrintf(m.action);
    if (m.isCtrl)  s += " Ctrl";
    if (m.isAlt)   s += " Alt";
    if (m.isShift) s += " Shift";
    s += ">";
    return s;
}

 // vaterm::InputPrintf(const InputData& e)
 //
 // 任意输入事件 → 显示串（键盘走 KeyPrintf，鼠标走 MousePrintf）。
 //
 // 参数：
 // const InputData& e ， 任意输入事件
 //
 // 返回:
 // std::string , 事件的书面表示；NON 返回 ""
 //
 // lc3124(260923):给用户一个统一的打印入口，键鼠通吃。
std::string InputPrintf(const InputData& e) {
    switch (e.inputType) {
    case InputData::InputType::KEYBD: return KeyPrintf(e.value.keybd);
    case InputData::InputType::MOUSE: return MousePrintf(e.value.mouse);
    case InputData::InputType::NON:   return "";
    }
    return "";
}

//- input_fsm 实现

 // vaterm::input_fsm::feed(unsigned char b, ...) — 单字节驱动入口
 //
 // 把当前状态对应的 step_* 跑一遍；若它返回 true，表示“这一个字节还没处理完、
 // 请在新状态下再来一次”，于是循环重跑，直到某个 step_* 返回 false。
 //
 // 参数：
 // unsigned char b ， 输入字节
 // std::deque<InputData>& out ， 事件输出队列
 //
 // 返回:
 // 无
void input_fsm::feed(unsigned char b, std::deque<InputData>& out) {
    bool reprocess = true;  // 这个字节还要不要（在新状态下）再处理一次？
    while (reprocess) {
        // 按当前状态派发到对应的 step_*；返回值决定是否重处理同一字节
        switch (state_) {
        case State::GROUND: reprocess = step_ground(b, out); break;
        case State::ESC:    reprocess = step_esc(b, out);    break;
        case State::CSI:    reprocess = step_csi(b, out);    break;
        case State::SS3:    reprocess = step_ss3(b, out);    break;
        case State::MOUSE:  reprocess = step_mouse(b, out);  break;
        case State::UTF8:   reprocess = step_utf8(b, out);   break;
        }
    }
}

 // vaterm::input_fsm::feed(std::string_view, ...) — 批量入口
 //
 // 逐个字节调用单字节版本，顺序与入参一致。
void input_fsm::feed(std::string_view bytes, std::deque<InputData>& out) {
    for (unsigned char b : bytes) feed(b, out);  // 逐字节投喂
}

// 是否停在半截序列里（非 GROUND）。调用方据此判断“还差后续字节”。
bool input_fsm::pending() const {
    return state_ != State::GROUND;
}

 // vaterm::input_fsm::flush_pending(...)
 //
 // lc3124(261013): 首字节为ESC的话，有可能是用户按下了ESC键，也有可能是终端发来了
 // 转义序列，如果接受到ESC后，迟迟没有拿到转义序列的结束标志，就一律收敛为ESC按键来
 // 处理。reprocess标志就是为了这个情况设计的。
void input_fsm::flush_pending(std::deque<InputData>& out) {
    if (state_ == State::ESC) {
        push_key(out, 0, InputData::KeyCode::ESC);
        reset();
    }
}

// 回到 GROUND 并清空累积字节；解析成功或序列失步时调用。
void input_fsm::reset() {
    state_ = State::GROUND;
    acc_.clear();
}

 // vaterm::input_fsm::push_key(...)
 //
 // 组装一个键盘事件并入队。用 InputData::makeKeybd 统一设置标签与负载，
 // 避免外部手写联合体。
 //
 // 参数：
 // cp    ， 可打印键的码点（特殊键传 0）
 // code  ， 特殊键码（普通键传 KeyCode::NONE）
 // shift/alt/ctrl ， 修饰位
void input_fsm::push_key(std::deque<InputData>& out, char32_t cp,
                         InputData::KeyCode code,
                         bool shift, bool alt, bool ctrl) {
    out.push_back(InputData::makeKeybd({cp, code, shift, alt, ctrl}));
}

 // vaterm::input_fsm::step_ground(...)
 //
 // GROUND不在任何序列之中,这个状态下，逐字节识别普通输入。
 // 绝大多数按键都在这里一次处理完；只有 ESC、UTF-8 首字节会把状态机推向别的状态。
 // 字节分类按优先级依次判断：
 // 转义(0x1B) > 具名控制键 > Ctrl 组合键 > 可打印 ASCII > UTF-8 首字节 > 其余丢弃。
 //
 // 参数：
 // unsigned char b ， 当前字节
 // std::deque<InputData>& out ， 事件输出队列
 //
 // 返回:
 // bool , 恒为 false ，GROUND 是收敛态，不会出现同一字节需在新状态再处理的
 //        情况（返回值约定见 step_* 系列总说明：true 代表请在新状态下重处理该字节）。
bool input_fsm::step_ground(unsigned char b, std::deque<InputData>& out) {
    using KC = InputData::KeyCode;

    // 0x1B = ESC：既可能是单独按下的 Esc 键，也可能是某个转义序列的开头，
    // 此刻仅凭一个字节无法区分。于是转入 ESC 态，把已见字节存进 acc_，
    // 交给后续字节、或超时时的 flush_pending() 来最终定夺。
    if (b == 0x1B) { state_ = State::ESC; acc_.assign(1, '\033'); return false; }

    // 三个有专属键名的控制字符
    if (b == 0x09) { push_key(out, 0, KC::TAB);       return false; }  // 制表符(Tab)
    if (b == 0x0D) { push_key(out, 0, KC::ENTER);     return false; }  // 回车(CR)
    if (b == 0x7F) { push_key(out, 0, KC::BACKSPACE); return false; }  // 退格(DEL)

    // 0x00 ， Ctrl+Space ：载荷用空格码点，修饰位标 Ctrl。
    if (b == 0x00) { push_key(out, U' ', KC::NONE, false, false, true); return false; }

    // 0x01-0x1A ， Ctrl+a .. Ctrl+z：
    // 终端里 Ctrl+字母 产生的就是该字母 ASCII 码减 0x40
    // 这里反过来求得字面原本序号就可以了
    if (b >= 0x01 && b <= 0x1A) {
        push_key(out, static_cast<char32_t>('a' + b - 1), KC::NONE, false, false, true);
        return false;
    }

    // 0x1C-0x1F ， Ctrl+\ Ctrl+] Ctrl+^ Ctrl+_：
    // 这几个不落在字母区间内，需要单独映射，同样由 ASCII 码减 0x40 得出。
    if (b >= 0x1C && b <= 0x1F) {
        char32_t cp = b == 0x1C ? U'\\' : b == 0x1D ? U']' : b == 0x1E ? U'^' : U'_';
        push_key(out, cp, KC::NONE, false, false, true);
        return false;
    }

    // 0x20-0x7E ，可打印 ASCII：字节值本身就是字符码点，直接产出一个无修饰键事件。
    if (b >= 0x20 && b <= 0x7E) {
        push_key(out, static_cast<char32_t>(b), KC::NONE);
        return false;
    }

    // 0xC2-0xF7 是 UTF-8 首字节区间，按首字节大小决定后续要几个字节：
    //   0xC2-0xDF 两字节、0xE0-0xEF 三字节（含 CJK）、0xF0-0xF7 四字节（emoji 等）。
    // 不含 0xC0/0xC1（属“过长编码”，非法），也不含 0xF8 及以上（非 UTF-8）。
    // 此处只把首字节存入 acc_ 并转入 UTF8 态，等续字节到齐后再解码出码点。
    if (b >= 0xC2 && b <= 0xF7) {
        state_ = State::UTF8;
        acc_.assign(1, static_cast<char>(b));
        return false;
    }

    // 其余字节（如 0x80-0xBF 的孤立续字节、0xF8-0xFF 等）无法识别，直接丢弃。
    return false;
}

 // vaterm::input_fsm::step_esc(...)
 //
 // ESC(0x1B) 之后到来的第一个字节，决定这条转义序列到底是哪一种：
 //   '['         CSI控制序列
 //   'O'         SS3
 //   0x20-0x7E   Alt+<该可打印字符>
 //   其它        认定最初的 ESC 是“单独按下的 Esc 键”，且当前字节不属于任何
 //                转义序列，返回 true 让它在 GROUND 态重新处理。
 //
 // 返回 true -> 需要新状态下重处理该字节（见 step_* 系列总说明）。
bool input_fsm::step_esc(unsigned char b, std::deque<InputData>& out) {
    // '['：转入 CSI；累积字节记下这个 '['（后面 parse_csi 要去头）
    if (b == '[') { state_ = State::CSI; acc_ += '['; return false; }
    // 'O'：转入 SS3
    if (b == 'O') { state_ = State::SS3; acc_ += 'O'; return false; }
    // 可打印字符：ESC + 可打印 = Alt+字符，产出一个带 Alt 修饰的键盘事件
    if (b >= 0x20 && b <= 0x7E) {
        push_key(out, static_cast<char32_t>(b), InputData::KeyCode::NONE, false, true, false);
        reset();
        return false;
    }
    // 其它字节：最初的 ESC 就是 Esc 键；先补发 Esc，再让当前字节回到 GROUND 重处理
    push_key(out, 0, InputData::KeyCode::ESC);
    reset();
    return true;
}

 // vaterm::input_fsm::step_csi(...)
 //
 // CSI 序列的累积态。逐步把字节追加到 acc_，直到出现终止字节。
 // CSI 的一般形式：ESC '[' <参数字节> <终止字节>，终止字节落在 0x40-0x7E。
 // NOTICE: 以 ESC '[' '<' 开头的是 SGR 鼠标序列，转入 MOUSE 态专门累积。
 //
 // 返回：恒为 false（本函数自己收敛）。
bool input_fsm::step_csi(unsigned char b, std::deque<InputData>& out) {
    acc_ += static_cast<char>(b);
    // 第 3 字节是 '<' ， 这是 SGR 鼠标序列 \033[<Cb;Cx;CyM/m，转 MOUSE 态
    if (acc_.size() == 3 && acc_[2] == '<') { state_ = State::MOUSE; return false; }
    // 终止字节：0x40-0x7E。此时 acc_ = "\033[<参数><终止>"
    if (b >= 0x40 && b <= 0x7E) {
        // 去掉头部的 "\033[" 和尾部的终止字节，中间即参数串
        std::string_view p(acc_);
        p.remove_prefix(2);
        p.remove_suffix(1);
        parse_csi(p, static_cast<char>(b), out);
        reset();
        return false;
    }
    if (acc_.size() > 64) reset();  // 失步保护：异常长的序列直接丢弃
    return false;
}

 // vaterm::input_fsm::step_ss3(...)
 //
 // SS3 序列：ESC 'O' <单字节>。常见于 xterm 的 F1-F4。
 // 本函数接收 ESC 'O' 之后的那个字节，映射成对应功能键。
 //
 // 返回：恒为 false。
bool input_fsm::step_ss3(unsigned char b, std::deque<InputData>& out) {
    using KC = InputData::KeyCode;
    // xterm 约定'P'..'S' 对应 F1..F4，其余暂不处理
    switch (b) {
    case 'P': push_key(out, 0, KC::F1); break;
    case 'Q': push_key(out, 0, KC::F2); break;
    case 'R': push_key(out, 0, KC::F3); break;
    case 'S': push_key(out, 0, KC::F4); break;
    default: break;  // 未识别的 SS3：静默忽略
    }
    reset();
    return false;
}

 // vaterm::input_fsm::step_mouse(...)
 //
 // SGR 鼠标序列的累积态：从 ESC '[' '<' 之后开始，一直累积到终止字节
 // 'M'（按下/滚轮/移动）或 'm'（松开），再交给 parse_mouse 解析。
 //
 // 返回：恒为 false。
bool input_fsm::step_mouse(unsigned char b, std::deque<InputData>& out) {
    acc_ += static_cast<char>(b);
    if (b == 'M' || b == 'm') {   // 序列结束，解析
        parse_mouse(acc_, out);
        reset();
        return false;
    }
    if (acc_.size() > 64) reset();  // 失步保护：一直等不到终止字节就丢弃
    return false;
}

 // vaterm::input_fsm::step_utf8(...)
 //
 // UTF-8 多字节字符的累积态。第一个字节（进入本态时已存入 acc_）决定总长度：
 //   0xC2-0xDF 共 2 字节
 //   0xE0-0xEF 共 3 字节（含绝大多数 CJK）
 //   0xF0-0xF7 共 4 字节（emoji、罕见字）
 // 每来一个续字节就校验它是否形如 10xxxxxx；到齐后按位拼出 Unicode 码点。
 //
 // 返回：恒为 false。
bool input_fsm::step_utf8(unsigned char b, std::deque<InputData>& out) {
    acc_ += static_cast<char>(b);
    auto lead = static_cast<unsigned char>(acc_[0]);
    // 由首字节反推总字节数
    int  need = lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
    // 逐个校验已收到的续字节：必须是 10xxxxxx（高两位为 10）
    for (int i = 1; i < static_cast<int>(acc_.size()); ++i) {
        if ((static_cast<unsigned char>(acc_[i]) & 0xC0) != 0x80) { reset(); return false; }
    }
    // 还没收齐：继续等后续字节
    if (static_cast<int>(acc_.size()) < need) return false;

    // 取续字节低 6 位（编码位）的小工具
    auto u = [&](int i) {
        return static_cast<char32_t>(static_cast<unsigned char>(acc_[i]) & 0x3F);
    };
    // 按 UTF-8 位布局拼回码点：
    //   2 字节：110xxxxx 10yyyyyy            → (lead&0x1F)<<6 | yyyyyy
    //   3 字节：1110xxxx 10yyyyyy 10zzzzzz   → (lead&0x0F)<<12 | ...
    //   4 字节：11110xxx 10yyyyyy 10zzzzzz 10wwwwww
    char32_t cp = 0;
    if (need == 2)      cp = ((lead & 0x1F) << 6) | u(1);
    else if (need == 3) cp = ((lead & 0x0F) << 12) | (u(1) << 6) | u(2);
    else                cp = ((lead & 0x07) << 18) | (u(1) << 12) | (u(2) << 6) | u(3);

    push_key(out, cp, InputData::KeyCode::NONE);
    reset();
    return false;
}

 // vaterm::input_fsm::parse_csi(...)
 //
 // 把累积阶段拿到的 CSI 参数串解析成键盘事件。
 // 形式：<参数> <终止字节>，其中 <参数> 可能为空（如方向键 "A"）、
 // 或形如 "n"（如 "~" 系功能键）、或 "n;m"（m 是修饰键编码）。
 //
 // 参数：
 // std::string_view params ， 去掉了 "\033[" 与终止字节的中间串
 // char final ， 终止字节（决定序列的“大类”）
 // std::deque<InputData>& out ， 事件输出队列
 //
 // 返回:
 // 无
void input_fsm::parse_csi(std::string_view params, char final,
                          std::deque<InputData>& out) {
    using KC = InputData::KeyCode;
    // 把（纯数字的）参数字段转成 int，遇到非数字即停
    auto to_int = [](std::string_view sv) {
        int v = 0;
        for (char c : sv) { if (c < '0' || c > '9') break; v = v * 10 + (c - '0'); }
        return v;
    };
    // 参数形如 "n" 或 "n;m"（m 为修饰键编码）。semi 是分号位置。
    auto semi   = params.find(';');
    int  main_n = semi == std::string_view::npos ? to_int(params)
                                                 : to_int(params.substr(0, semi));
    int  mod    = semi == std::string_view::npos ? 0 : to_int(params.substr(semi + 1));
    // xterm 修饰键编码：mod-1 的三个 bit 依次表示 Shift / Alt / Ctrl
    bool shift  = mod != 0 && ((mod - 1) & 1) != 0;
    bool alt    = mod != 0 && ((mod - 1) & 2) != 0;
    bool ctrl   = mod != 0 && ((mod - 1) & 4) != 0;

    KC code = KC::NONE;
    if (params.empty()) {
        // 无参数：由终止字节直接决定方向键 / Home / End
        switch (final) {
        case 'A': code = KC::UP;    break;
        case 'B': code = KC::DOWN;  break;
        case 'C': code = KC::RIGHT; break;
        case 'D': code = KC::LEFT;  break;
        case 'H': code = KC::HOME;  break;
        case 'F': code = KC::END;   break;
        default: return;            // 未识别 → 忽略
        }
    } else if (final == '~') {
        // "~" 系：由第一个参数 main_n 决定是哪个键（xterm 数字键码）
        switch (main_n) {
        case 1:  code = KC::HOME; break;
        case 2:  code = KC::INS;  break;
        case 3:  code = KC::DEL;  break;
        case 4:  code = KC::END;  break;
        case 5:  code = KC::PGUP; break;
        case 6:  code = KC::PGDN; break;
        case 11: code = KC::F1;   break;
        case 12: code = KC::F2;   break;
        case 13: code = KC::F3;   break;
        case 14: code = KC::F4;   break;
        case 15: code = KC::F5;   break;
        case 17: code = KC::F6;   break;
        case 18: code = KC::F7;   break;
        case 19: code = KC::F8;   break;
        case 20: code = KC::F9;   break;
        case 21: code = KC::F10;  break;
        case 23: code = KC::F11;  break;
        case 24: code = KC::F12;  break;
        default: return;            // 未识别 → 忽略
        }
    } else {
        return;                         // 其它终止字节暂不支持
    }
    push_key(out, 0, code, shift, alt, ctrl);
}

 // vaterm::input_fsm::parse_mouse(...)
 //
 // 解析一条完整的 SGR 鼠标序列，形如：
 //     ESC [ < Cb ; Cx ; Cy M|m
 //
 // 参数：
 // const std::string& seq ， 完整序列（含头尾）
 // std::deque<InputData>& out ， 事件输出队列
 //
 // 返回:
 // 无，格式不合法则静默丢弃
 //
 // lc3124(261013): 太难看了，能跑就差不多了。
void input_fsm::parse_mouse(const std::string& seq, std::deque<InputData>& out) {
    // 先做头部/尾部格式校验：至少 6 字节且形如 ESC [ <
    if (seq.size() < 6 || seq[0] != '\033' || seq[1] != '[' || seq[2] != '<') return;
    char term = seq.back();
    if (term != 'M' && term != 'm') return;   // 'M'=按下类，'m'=松开

    // 定位两个分号，把 Cb / Cx / Cy 三段切出来
    auto p1 = seq.find(';', 3);
    if (p1 == std::string::npos) return;
    auto p2 = seq.find(';', p1 + 1);
    if (p2 == std::string::npos) return;

    // 纯数字字段
    auto to_int = [](std::string_view sv) {
        int v = 0;
        for (char c : sv) { if (c < '0' || c > '9') break; v = v * 10 + (c - '0'); }
        return v;
    };
    int cb  = to_int(std::string_view(seq).substr(3, p1 - 3));                    // Cb
    int col = to_int(std::string_view(seq).substr(p1 + 1, p2 - p1 - 1));          // Cx
    int row = to_int(std::string_view(seq).substr(p2 + 1, seq.size() - p2 - 2));  // Cy

    InputData::MouseData m;
    // 终端坐标从 1 起，规范到本模块的 0 起
    m.col     = col - 1;
    m.row     = row - 1;
    // Cb 的 bit2/3/4 分别是 Shift/Alt/Ctrl
    m.isShift = (cb & 4) != 0;
    m.isAlt   = (cb & 8) != 0;
    m.isCtrl  = (cb & 16) != 0;

    // Cb 低 2 位表示按键编号 0/1/2 → 左/中/右；3 表示“无具体按键”
    auto to_btn = [](int b) {
        return b == 0 ? InputData::MouseButton::LEFT
             : b == 1 ? InputData::MouseButton::MIDDLE
             : b == 2 ? InputData::MouseButton::RIGHT
                      : InputData::MouseButton::NONE;
    };
    int btn = cb & 3;
    if (cb & 64) {                       // bit6=1：滚轮；最低位区分上/下
        m.button = InputData::MouseButton::NONE;
        m.action = (cb & 1) ? InputData::MouseAction::SCROLL_DOWN
                            : InputData::MouseAction::SCROLL_UP;
    } else if (cb & 32) {                // bit5=1：移动；btn==3 视为无按键悬停(MOVE)，否则拖动(DRAG)
        m.button = to_btn(btn);
        m.action = (btn == 3) ? InputData::MouseAction::MOVE
                              : InputData::MouseAction::DRAG;
    } else {                             // 其余：按下 / 松开
        m.button = to_btn(btn);
        m.action = (term == 'm' || btn == 3) ? InputData::MouseAction::RELEASE
                                             : InputData::MouseAction::PRESS;
    }
    out.push_back(InputData::makeMouse(m));
}

//- input 实现

// 默认构造：各成员由类内初值初始化（未启动、状态 OFF）。
input::input() = default;

// 析构即 stop()，确保后台线程被回收、终端被还原（stop 幂等，可安全重复调用）。
input::~input() { stop(); }


 // vaterm::input::start()
 //
 // 正式接管终端输入：
 //   1) 要求调用方已把终端切到 raw 模式（本模块不负责切）；
 //   2) 打开鼠标追踪（1000/1002/1006 三个 DECSET）；
 //   3) 清空缓冲、递增世代（让旧 dumper 知道游标失效）；
 //   4) 启动后台解析线程 reader_loop()。
 // 幂等：已启动时直接返回 true。
 //
 // 返回：成功 true；终端不在 raw 模式则 false。
bool input::start() {
    std::lock_guard<std::mutex> lk(lifecycle_);   // 串行化 start/stop
    if (running_) return true;                     // 已启动

    // 终端必须已经处于 raw 模式（非规范模式）：否则不接管输入，直接返回 false。
    struct termios tio{};
    if (::tcgetattr(STDIN_FILENO, &tio) != 0 || (tio.c_lflag & ICANON) != 0) {
        state_ = InputMode::INPUT_DISABLE;
        return false;
    }

    // 开启鼠标追踪：1000=按下/松开上报，1002=按住移动（拖动）上报，1006=SGR 扩展坐标
    terminal::write("\033[?1000h\033[?1002h\033[?1006h");
    terminal::flush();
    started_ = true;

    // 清空历史缓冲并推进世代：所有已存在的 dumper 游标需重对齐
    {
        std::lock_guard<std::mutex> lock(m_);
        buffer_.clear();
        seq_ = 0;
        ++generation_;          // 引擎已清空：所有 dumper 需重对齐游标
    }

    if (reader_.joinable()) reader_.join();   // 回收可能已结束的旧线程
    mouse_enabled_ = true;
    keybd_enabled_ = true;
    running_       = true;
    reader_        = std::thread([this] { reader_loop(); });   // 后台解析线程
    state_         = InputMode::ON;
    return true;
}

 // vaterm::input::stop()
 //
 // 交还输入：置停止标志并唤醒等待者 → 回收后台线程 → 清空缓冲、推进世代 →
 // 关闭鼠标追踪（仅当此前确实 start 过，避免打扰终端）。幂等。
 //
 // 返回：恒为 true。
bool input::stop() {
    std::lock_guard<std::mutex> lk(lifecycle_);
    running_ = false;          // 通知 reader_loop 退出
    cv_.notify_all();          // 唤醒 waitInput / dump 的等待者
    if (reader_.joinable()) reader_.join();   // 等线程真正结束
    {
        std::lock_guard<std::mutex> lock(m_);
        buffer_.clear();
        seq_ = 0;
        ++generation_;          // 引擎已清空：所有 dumper 需重对齐游标
    }
    // 写终止序列，如果没有跑过引擎就不打扰终端。
    if (started_) {
        terminal::write("\033[?1006l\033[?1002l\033[?1000l");  // 关闭鼠标追踪（与 start 相反）
        terminal::flush();
        started_ = false;
    }
    state_ = InputMode::OFF;
    return true;
}

// 开启鼠标追踪：向终端写 DECSET 序列，并同步刷新 getState()。未启动时按契约返回 false。
bool input::enableMouse() {
    if (!running_) return false;
    terminal::write("\033[?1000h\033[?1002h\033[?1006h");
    terminal::flush();
    mouse_enabled_ = true;
    update_state();
    return true;
}

// 关闭鼠标追踪（DECRST 序列），与 enableMouse 对称。
bool input::disableMouse() {
    if (!running_) return false;
    terminal::write("\033[?1006l\033[?1002l\033[?1000l");
    terminal::flush();
    mouse_enabled_ = false;
    update_state();
    return true;
}

// 开启键盘事件（纯软件开关：事件仍被解析，只是允许入缓冲）。未启动时返回 false。
bool input::enableKeyBd() {
    if (!running_) return false;
    keybd_enabled_ = true;
    update_state();
    return true;
}

// 关闭键盘事件：enqueue 时会丢弃键盘事件（见 input::enqueue）。
bool input::disableKeyBd() {
    if (!running_) return false;
    keybd_enabled_ = false;
    update_state();
    return true;
}

// 原子读取当前工作状态（线程安全，可随时调用）。
input::InputMode input::getState() const {
    return state_.load();
}

// 由“键盘是否启用 × 鼠标是否启用”组合出对外的 InputMode。
void input::update_state() {
    bool k  = keybd_enabled_;
    bool mo = mouse_enabled_;
    state_  = (k && mo)  ? InputMode::ON
            : (k && !mo) ? InputMode::MOUSE_DISABLE
            : (!k && mo) ? InputMode::KEYBD_DISABLE
                         : InputMode::INPUT_DISABLE;
}


 // vaterm::input::reader_loop()
 //
 // 后台解析线程主体。循环做三件事：
 //   1) poll(stdin, 50ms) 等可读；
 //   2) 把此刻确实可读的字节全部读出，喂给状态机 fsm_，产出事件；
 //   3) 把产出的事件 enqueue 进环形缓冲，并唤醒消费者。
 // 关键点：
 //   - poll 超时(50ms) 会 flush_pending()，把半截的孤立 ESC 收敛成 Esc 键；
 //   - 排空字节前先用 poll(...,0) 预检，避免 read_byte() 在非 raw/管道上阻塞；
 //   - 识别到 HUP/ERR/NVAL，或“可读却反复读不到”，判定输入源结束并停止。
 //
 // lc3124(261012): 这一段代码不是我写的，我需要花一些时间来理清楚这玩意。
void input::reader_loop() {
    std::deque<InputData> produced;   // 本轮解析出的事件暂存
    int idle_ready = 0;   // 连续“poll 说可读却读不到”的次数，用于识别 EOF
    while (running_) {
        struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
        int r = ::poll(&pfd, 1, 50);  // 最多等 50ms
        if (r < 0) continue;          // 被信号打断等：重试

         // 如果终端发来escape序列，按理来说应该读到序列结束，但是如果esc序列
         // 长时间没有终止，那就不等来，直接收敛
        if (r == 0) {                 // 超时：收敛孤立的 Esc
            idle_ready = 0;
            fsm_.flush_pending(produced);
            enqueue(produced);
            continue;
        }

        // 只排空“此刻确实可读”的字节：用 poll(0) 预检，
        // 避免 read_byte() 在非 raw / 管道下阻塞住。
        bool got_any = false;
        for (;;) {
            struct pollfd q = {STDIN_FILENO, POLLIN, 0};
            if (::poll(&q, 1, 0) <= 0) break;   // 暂时没数据了 → 停
            auto b = terminal::read_byte();
            if (!b) break;
            fsm_.feed(*b, produced);            // 逐字节喂状态机
            got_any = true;
        }
        enqueue(produced);                      // 入缓冲并唤醒消费者

        // 输入源结束：HUP/ERR/NVAL，或反复“可读却无数据”。
        if (pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
            running_ = false;
            state_   = InputMode::INPUT_DISABLE;
            cv_.notify_all();
            break;
        }
        if (got_any) {
            idle_ready = 0;
        } else if (++idle_ready > 3) {          // 连续多次“可读却空”，认定 EOF
            running_ = false;
            state_   = InputMode::INPUT_DISABLE;
            cv_.notify_all();
            break;
        }
    }
}

 // 把这一轮解析出的事件写进共享环形缓冲，并唤醒消费者：
 //   - 键盘被禁用时丢弃 KEYBD 事件（其它类型照常）；
 //   - 每条事件分配递增序号 seq_，作为 dumper 游标的依据；
 //   - 缓冲超长时从头部丢最旧的（保留最近 VATERM_INPUT_BUFFER_SIZE 条）。
 // 只在真正入队成功时 notify，避免无谓唤醒。
void input::enqueue(std::deque<InputData>& produced) {
    if (produced.empty()) return;
    bool pushed = false;
    {
        std::lock_guard<std::mutex> lock(m_);
        bool keep_key = keybd_enabled_;   // 快照一次，循环内保持一致
        for (auto& e : produced) {
            if (!keep_key && e.inputType == InputData::InputType::KEYBD) continue;  // 键盘已禁用 → 丢弃
            buffer_.push_back(Entry{++seq_, std::move(e)});   // 分配序号并转入缓冲
            pushed = true;
        }
        // 环形语义：超出容量丢最旧的
        while (buffer_.size() > VATERM_INPUT_BUFFER_SIZE)
            buffer_.pop_front();
        produced.clear();
    }
    if (pushed) cv_.notify_all();
}

// 判定数据包是否匹配想要的类型：type==NON 表示“不限类型”。
bool input::match(InputData::InputType type, const InputData& d) {
    return type == InputData::InputType::NON || d.inputType == type;
}

// 在缓冲里从最新往回找第一条匹配项（调用方需持 m_）。找不到返回空包(NON)。
InputData input::latest(InputData::InputType type) const {
    for (auto it = buffer_.rbegin(); it != buffer_.rend(); ++it)
        if (match(type, it->data)) return it->data;
    return InputData{};
}

// getInput(type)：非阻塞取“最新一条匹配事件”，不消费、不推进任何游标。
InputData input::getInput(InputData::InputType type) {
    std::lock_guard<std::mutex> lock(m_);
    return latest(type);
}

// getInput()：不限类型，取最新一条。
InputData input::getInput() {
    std::lock_guard<std::mutex> lock(m_);
    return latest(InputData::InputType::NON);
}

 // vaterm::input::waitInput(...)
 //
 // 阻塞等待“新出现的”匹配事件（不消费）。判据是缓冲末尾的序号 > 进入函数时的 seq_，
 // 即等待期间必须真的有新事件入队；仅缓冲里存在旧匹配项不算。
 // timeout_ms < 0 无限等待；超时或引擎被 stop 返回空包(NON)。
InputData input::waitInput(InputData::InputType type, int timeout_ms) {
    std::unique_lock<std::mutex> lock(m_);
    const std::uint64_t start = seq_;   // 进入时的写入序号
    // “最近更新的那一条”既要是新的（序号 > start），类型也要匹配。
    auto newest_matches = [&] {
        return !buffer_.empty() && buffer_.back().seq > start
               && match(type, buffer_.back().data);
    };
    auto ready = [&] { return !running_ || newest_matches(); };

    if (timeout_ms < 0) {
        cv_.wait(lock, ready);
    } else if (!cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), ready)) {
        return InputData{};                 // 超时 → NON
    }
    if (!running_ || !newest_matches()) return InputData{};   // 被 stop / 最新不是要的
    return buffer_.back().data;             // 返回最近更新的那一条
}

// waitInput(timeout_ms)：不限类型。
InputData input::waitInput(int timeout_ms) {
    return waitInput(InputData::InputType::NON, timeout_ms);
}


 // 倾倒器实现

// 构造：把游标对齐到该 input 的当前最新序号——只倾倒此后新产生的事件。
input::dumper::dumper(input& in) : in_(&in) {
    std::lock_guard<std::mutex> lock(in_->m_);
    cursor_     = in_->seq_;
    generation_ = in_->generation_;
}

// 从旧到新，找游标之后“最旧”的一条（需持 in_->m_）。
const input::Entry* input::dumper::next_unread() {
    for (const auto& e : in_->buffer_)
        if (e.seq > cursor_) return &e;
    return nullptr;
}

 // vaterm::input::dumper::dump(...)
 //
 // 取“游标之后最旧的那条”未读事件并前进游标。若已追平最新则等待新事件。
 // 若等待期间引擎换代（start/stop 清空过缓冲），先把游标重对齐到最新，
 // 避免旧游标指向不存在的序号。
 // timeout_ms < 0 无限等待；超时或引擎停止返回空包(NON)。
InputData input::dumper::dump(int timeout_ms) {
    std::unique_lock<std::mutex> lock(in_->m_);

    // 引擎被清空/重启,世代变了,游标重对齐到最新，避免“冲到未来”卡死。
    if (generation_ != in_->generation_) {
        cursor_     = in_->seq_;
        generation_ = in_->generation_;
    }

    auto ready = [&] { return !in_->running_ || next_unread() != nullptr; };
    if (!ready()) {
        if (timeout_ms < 0) {  ///< 无时限：一直等到 ready() 为真（有未读事件，或引擎停止）
            in_->cv_.wait(lock, ready);
        } else if (!in_->cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), ready)) {
            return InputData{};        // 超时 → NON
        }
    }

    const Entry* e = next_unread();
    if (!e) return InputData{};        // 被 stop / 无新事件
    if (e->seq > cursor_ + 1)          // 中间有被环形覆盖的 → 记丢帧
        lost_ += e->seq - cursor_ - 1;
    cursor_ = e->seq;                  // 游标前进到这一条
    return e->data;
}

// 重置：游标回到当前最新（忽略此前积压），并清零丢帧计数。
void input::dumper::reset() {
    std::lock_guard<std::mutex> lock(in_->m_);
    cursor_     = in_->seq_;
    generation_ = in_->generation_;
    lost_       = 0;
}

// 返回自构造 / 上次 reset 以来被环形缓冲覆盖而丢掉的事件条数。
std::uint64_t input::dumper::lost() const {
    std::lock_guard<std::mutex> lock(in_->m_);
    return lost_;
}

} // namespace vaterm
