//- vawk_lifecycle — 演示 vawk 的持有协议与实体生命周期
//
//  用 3 个固定的 System 持有者（A / B / C）、1 个常驻 EventRouter 和
//  4 个事件类型，把下面几条路径都跑一遍：
//
//    - make ent / enable：强持有（pending_）→ 弱持有（live_）的两阶段表
//    - listen：订阅即持有（Router 以 HolderKind::Router 登记）
//    - hold / disown：System 持有者决定实体是否算孤儿
//    - keep：用 set_release_policy() 覆盖默认的孤儿判定
//    - emit：事件经 Router → 实体队列 → 实体线程的 on_event()
//
//  运行：  printf 'help\nquit\n' | ./vawk_lifecycle
//
//  所有输出共用一个全局互斥，事件线程与命令线程不会互相打断。

#include <vawk/core/event_router.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

//- 事件定义

VAWK_EVENT_DEFINE(EventA)
VAWK_EVENT_DEFINE_END

VAWK_EVENT_DEFINE(EventB)
VAWK_EVENT_DEFINE_END

VAWK_EVENT_DEFINE(EventC)
VAWK_EVENT_DEFINE_END

VAWK_EVENT_DEFINE(EventD)
VAWK_EVENT_DEFINE_END

namespace {

//- 输出

std::mutex g_out_mtx;

// 输出到管道/文件时关掉颜色，保证脚本拿到的仍是纯文本。
static bool g_color = isatty(STDOUT_FILENO) != 0;

enum class Color { None, Green, Yellow, Red, Cyan, Bold, Dim };

// 取 SGR 参数；Color::None 不参与包装。
const char* sgr(Color color) {
    switch (color) {
        case Color::Green: return "32";
        case Color::Yellow: return "33";
        case Color::Red: return "31";
        case Color::Cyan: return "36";
        case Color::Bold: return "1";
        case Color::Dim: return "2";
        case Color::None: break;
    }
    return "";
}

// 非 TTY 或 Color::None 时原样返回；否则整段包一层 SGR。
std::string paint(const std::string& text, Color color) {
    if (!g_color || color == Color::None) return text;
    return std::string("\033[") + sgr(color) + "m" + text + "\033[0m";
}

// 全局串行化输出：多行内容拼成一个字符串，保证整段不被别的线程插队。
void say(const std::string& text, Color color = Color::None) {
    std::lock_guard<std::mutex> lk(g_out_mtx);
    std::cout << paint(text, color) << std::endl;
}

// 提示符只在标准输出是 TTY 时打印，避免污染管道输出。
void print_prompt() {
    if (!isatty(STDOUT_FILENO)) return;
    std::lock_guard<std::mutex> lk(g_out_mtx);
    std::cout << (g_color ? "\033[1;36m>> \033[0m" : ">> ");
    std::cout.flush();
}

// 用逗号拼接；空列表返回 fallback。
std::string join(const std::vector<std::string>& items, const std::string& sep,
                 const std::string& fallback = "(none)") {
    if (items.empty()) return fallback;
    std::string out = items.front();
    for (std::size_t i = 1; i < items.size(); ++i) out += sep + items[i];
    return out;
}

//- 事件名映射

struct EventDesc {
    std::string name;
    std::size_t id;
};

std::vector<EventDesc> event_table() {
    return {
        {"A", vawk::event_type_id<EventA>()},
        {"B", vawk::event_type_id<EventB>()},
        {"C", vawk::event_type_id<EventC>()},
        {"D", vawk::event_type_id<EventD>()},
    };
}

const EventDesc* find_event(const std::string& name) {
    static const auto table = event_table();
    for (const auto& d : table)
        if (d.name == name) return &d;
    return nullptr;
}

// 按名字造一个具体事件；名字非法时返回空指针。
std::shared_ptr<vawk::event::Base> make_event(const std::string& name) {
    if (name == "A") return std::make_shared<EventA>();
    if (name == "B") return std::make_shared<EventB>();
    if (name == "C") return std::make_shared<EventC>();
    if (name == "D") return std::make_shared<EventD>();
    return nullptr;
}

//- 持有者

// Holder 的登记接口是 protected，派生类把它暴露成 hold / unhold。
class NamedHolder : public vawk::Holder {
  public:
    explicit NamedHolder(std::string name)
        : name_(std::move(name)) {}

    const std::string& name() const { return name_; }

    void hold(const std::shared_ptr<vawk::Entity>& entity) {
        acquire(entity, vawk::HolderKind::System);
    }

    void unhold(const std::shared_ptr<vawk::Entity>& entity) {
        disown(entity);
    }

  private:
    std::string name_;
};

//- 实体

class DemoEntity : public vawk::Entity {
  public:
    DemoEntity(std::shared_ptr<vawk::EventRouter> router, std::string label)
        : Entity(router)
        , router_(std::move(router)) {
        set_label(std::move(label));
    }

    ~DemoEntity() override { say("entity released: " + label(), Color::Yellow); }

    // 未启用时只记账，enable() 里的 on_subscribe() 会统一订阅；
    // 已启用则立刻登记到 Router。
    void listen(std::size_t event_id) {
        pending_events_.push_back(event_id);
        if (enabled())
            if (auto self = weak_from_this().lock())
                router_->subscribe(event_id, self);
    }

  protected:
    // 一阶段订阅：把构造以来攒下的事件类型全部登记给 Router。
    bool on_subscribe(vawk::EventUpstream& up) override {
        for (auto id : pending_events_) up.subscribe(id, shared_from_this());
        return true;
    }

    void on_event(std::shared_ptr<vawk::event::Base>) override {
        say("processed event: " + label() + " !", Color::Cyan);
    }

  private:
    std::shared_ptr<vawk::EventRouter> router_;
    std::vector<std::size_t>           pending_events_; ///< enable 前攒下的订阅
};

//- 全局状态

std::shared_ptr<vawk::EventRouter>      g_router;
std::map<std::string, std::shared_ptr<DemoEntity>> g_pending; ///< enable 前：强持有
std::map<std::string, std::weak_ptr<DemoEntity>>   g_live;    ///< enable 后：弱持有
NamedHolder g_a{"A"};
NamedHolder g_b{"B"};
NamedHolder g_c{"C"};

// 按名字取实体：先找强持有表，再提升弱持有表。
std::shared_ptr<DemoEntity> find_entity(const std::string& name) {
    auto p = g_pending.find(name);
    if (p != g_pending.end()) return p->second;
    auto l = g_live.find(name);
    if (l != g_live.end()) return l->second.lock();
    return nullptr;
}

NamedHolder* find_holder(const std::string& name) {
    if (name == "A") return &g_a;
    if (name == "B") return &g_b;
    if (name == "C") return &g_c;
    return nullptr;
}

// 把 Holder 裸指针翻译成人名；Router 与未知持有者单独标注。
std::string holder_name(const vawk::Holder* owner) {
    if (owner == &g_a) return "A";
    if (owner == &g_b) return "B";
    if (owner == &g_c) return "C";
    if (owner == static_cast<vawk::Holder*>(g_router.get())) return "Router";
    return "?";
}

std::string entity_label(const std::shared_ptr<vawk::Entity>& entity) {
    return entity ? entity->label() : std::string{};
}

//- 命令实现

// 帮助行：命令名补齐到 25 字节，说明从第 27 列开始。
std::string help_line(const std::string& name, const std::string& desc) {
    std::string padded = name;
    if (padded.size() < 25) padded.append(25 - padded.size(), ' ');
    return "  " + paint(padded, Color::Cyan) + desc;
}

void cmd_help() {
    say(
        std::string("vawk lifecycle demo — 持有协议与生命周期\n") +
        "commands:\n" +
        help_line("help", "显示本帮助\n") +
        help_line("make ent <name>", "创建实体（pending_ 强持有）\n") +
        help_line("enable <name>", "启用实体并转入 live_（weak）\n") +
        help_line("listen <ent> <evt>", "订阅事件 A/B/C/D；未 enable 则延后订阅\n") +
        help_line("status <ent>", "显示实体的 label / holder / is_on\n") +
        help_line("status router", "显示 A/B/C/D 各自的订阅者\n") +
        help_line("hold <ent> A [B ...]", "以 System 身份持有实体\n") +
        help_line("disown <ent> A [B ...]", "每个 holder 放手\n") +
        help_line("list entity", "列出 live_ 中仍存活的实体\n") +
        help_line("list event", "列出 A/B/C/D 及订阅数\n") +
        help_line("keep <ent> [A ...]", "设置保活集合；不带名字恢复默认策略\n") +
        help_line("emit <evt>", "推送事件 A/B/C/D\n") +
        help_line("quit | exit", "退出"));
}

void cmd_make(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        say("usage: make ent <name>", Color::Red);
        return;
    }
    const std::string& name = args.front();

    auto l = g_live.find(name);
    if (l != g_live.end() && !l->second.expired()) {
        say("entity already exists: " + name, Color::Red);
        return;
    }
    if (g_pending.count(name) != 0) {
        say("entity already exists: " + name, Color::Red);
        return;
    }

    g_pending[name] = std::make_shared<DemoEntity>(g_router, name);
    say("created: " + name, Color::Green);
}

void cmd_enable(const std::string& name) {
    auto it = g_pending.find(name);
    if (it == g_pending.end()) {
        say("no pending entity: " + name, Color::Red);
        return;
    }

    // 顺序很关键：先 enable()，再移表，最后让局部 shared_ptr 离开作用域。
    // 若实体在 enable() 时被判孤儿，析构会发生在命令返回处，打印可见。
    auto       e  = it->second;
    const bool ok = e->enable();
    g_pending.erase(it);
    if (ok) g_live[name] = e;

    say(ok ? "enabled: " + name : "enable failed: " + name,
        ok ? Color::Green : Color::Red);
}

void cmd_listen(const std::string& ent, const std::string& evt) {
    auto e = find_entity(ent);
    if (!e) {
        say("entity not found: " + ent, Color::Red);
        return;
    }
    const EventDesc* d = find_event(evt);
    if (!d) {
        say("unknown event: " + evt, Color::Red);
        return;
    }
    e->listen(d->id);
    say(ent + " listens " + d->name, Color::Green);
}

void cmd_status_entity(const std::string& name) {
    auto e = find_entity(name);
    if (!e) {
        say("entity not found: " + name, Color::Red);
        return;
    }

    std::vector<std::string> holders;
    for (const auto& h : e->holder_snapshot()) holders.push_back(holder_name(h.owner));

    std::ostringstream os;
    os << paint("Entity:", Color::Bold) << "\n"
       << paint("  label : ", Color::Dim) << e->label() << "\n"
       << paint("  holder : ", Color::Dim) << join(holders, ", ") << "\n"
       << paint("  is_on : ", Color::Dim) << (e->enabled() ? "true" : "false");
    say(os.str());
}

void cmd_status_router() {
    std::ostringstream os;
    os << paint("Router:", Color::Bold);
    for (const auto& d : event_table()) {
        std::vector<std::string> labels;
        for (const auto& sub : g_router->subscribers(d.id)) labels.push_back(entity_label(sub));
        os << "\n  " << d.name << " -> " << join(labels, ", ");
    }
    say(os.str());
}

void cmd_hold(const std::string& ent, const std::vector<std::string>& names) {
    auto e = find_entity(ent);
    if (!e) {
        say("entity not found: " + ent, Color::Red);
        return;
    }
    if (names.empty()) {
        say("usage: hold <ent> A [B ...]", Color::Red);
        return;
    }

    for (const auto& n : names) {
        auto* h = find_holder(n);
        if (!h) {
            say("unknown holder: " + n, Color::Red);
            continue;
        }
        h->hold(e);
        say(ent + " held by " + n, Color::Green);
    }
}

void cmd_disown(const std::string& ent, const std::vector<std::string>& names) {
    auto e = find_entity(ent);
    if (!e) {
        say("entity not found: " + ent, Color::Red);
        return;
    }
    if (names.empty()) {
        say("usage: disown <ent> A [B ...]", Color::Red);
        return;
    }

    for (const auto& n : names) {
        auto* h = find_holder(n);
        if (!h) {
            say("unknown holder: " + n, Color::Red);
            continue;
        }
        h->unhold(e);
        say(ent + " disowned by " + n, Color::Green);
    }
}

void cmd_list_entity() {
    for (const auto& entry : g_live) {
        auto e = entry.second.lock();
        if (!e) continue;  // expired：已经释放，跳过
        say("entity " + e->label() + (e->enabled() ? " (is_on=true)" : " (released)"));
    }
}

void cmd_list_event() {
    for (const auto& d : event_table())
        say(d.name + " : " + std::to_string(g_router->subscribers(d.id).size()) + " subscriber(s)");
}

void cmd_keep(const std::string& ent, const std::vector<std::string>& names) {
    auto e = find_entity(ent);
    if (!e) {
        say("entity not found: " + ent, Color::Red);
        return;
    }

    // 不带名字：传空 ReleasePolicy，恢复默认的 is_orphan() 判定。
    if (names.empty()) {
        e->set_release_policy({});
        say("keep " + ent + ": default", Color::Green);
        return;
    }

    std::vector<vawk::Holder*> keepers;
    std::vector<std::string>   accepted;
    for (const auto& n : names) {
        auto* h = find_holder(n);
        if (!h) {
            say("unknown holder: " + n, Color::Red);
            continue;
        }
        keepers.push_back(h);
        accepted.push_back(n);
    }
    if (keepers.empty()) return;

    // 策略在实体持 mtx_holders_ 时调用：只比较裸指针，不加锁、不回调。
    e->set_release_policy([keepers](const std::vector<vawk::HolderInfo>& hs) {
        for (const auto& h : hs)
            for (auto* k : keepers)
                if (h.owner == k) return false;  // 有保活者 → 不释放
        return true;
    });
    say("keep " + ent + ": " + join(accepted, ", "), Color::Green);
}

void cmd_emit(const std::string& evt) {
    auto event = make_event(evt);
    if (!event) {
        say("unknown event: " + evt, Color::Red);
        return;
    }
    g_router->push(std::move(event));
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
}

//- 命令分发

void dispatch(const std::string& cmd, const std::vector<std::string>& args) {
    if (cmd == "help") {
        cmd_help();
    } else if (cmd == "make") {
        if (args.empty() || args.front() != "ent") {
            say("usage: make ent <name>", Color::Red);
            return;
        }
        cmd_make({args.begin() + 1, args.end()});
    } else if (cmd == "enable") {
        if (args.size() != 1) {
            say("usage: enable <name>", Color::Red);
            return;
        }
        cmd_enable(args.front());
    } else if (cmd == "listen") {
        if (args.size() != 2) {
            say("usage: listen <ent> <evt>", Color::Red);
            return;
        }
        cmd_listen(args[0], args[1]);
    } else if (cmd == "status") {
        if (args.size() != 1) {
            say("usage: status <ent> | status router", Color::Red);
            return;
        }
        if (args.front() == "router")
            cmd_status_router();
        else
            cmd_status_entity(args.front());
    } else if (cmd == "hold") {
        if (args.size() < 2) {
            say("usage: hold <ent> A [B ...]", Color::Red);
            return;
        }
        cmd_hold(args.front(), {args.begin() + 1, args.end()});
    } else if (cmd == "disown") {
        if (args.size() < 2) {
            say("usage: disown <ent> A [B ...]", Color::Red);
            return;
        }
        cmd_disown(args.front(), {args.begin() + 1, args.end()});
    } else if (cmd == "list") {
        if (args.size() != 1 || (args.front() != "entity" && args.front() != "event")) {
            say("usage: list entity | list event", Color::Red);
            return;
        }
        if (args.front() == "entity")
            cmd_list_entity();
        else
            cmd_list_event();
    } else if (cmd == "keep") {
        if (args.empty()) {
            say("usage: keep <ent> [A ...]", Color::Red);
            return;
        }
        cmd_keep(args.front(), {args.begin() + 1, args.end()});
    } else if (cmd == "emit") {
        if (args.size() != 1) {
            say("usage: emit <evt>", Color::Red);
            return;
        }
        cmd_emit(args.front());
    } else {
        say("unknown command: " + cmd + "（输入 help 查看命令）", Color::Red);
    }
}

}  // namespace

int main() {
    g_router = std::make_shared<vawk::EventRouter>();
    g_router->start_event_loop();

    std::string line;
    for (;;) {
        print_prompt();
        if (!std::getline(std::cin, line)) break;

        std::istringstream iss(line);
        std::string        cmd;
        if (!(iss >> cmd)) continue;

        std::vector<std::string> args;
        for (std::string token; iss >> token;) args.push_back(std::move(token));

        if (cmd == "quit" || cmd == "exit") break;
        dispatch(cmd, args);
    }

    // 先停 Router 线程，避免收尾阶段还有事件在派发。
    g_router->stop_event_loop();
    g_pending.clear();
    g_live.clear();
    return 0;
}
