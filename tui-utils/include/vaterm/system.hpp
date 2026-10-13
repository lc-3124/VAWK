#ifndef VATERM_SYSTEM_HPP
#define VATERM_SYSTEM_HPP

// -----------------------------------------------------------------------
//  vaterm::sys — 系统信息辅助
//
//  一组 POSIX 封装，用于返回基础的 OS 级信息。
//  所有函数均无状态且线程安全（可重入）。
//
//  函数：
//    user_name()         — 当前用户的登录名
//    host_name()         — 系统主机名（nodename）
//    env(key)            — 环境变量的值
//    cwd()               — 当前工作目录
//    exec(command)       — 执行命令并捕获其 stdout
// -----------------------------------------------------------------------

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <pwd.h>

namespace vaterm {
namespace sys {

// 返回当前用户的登录名。
//
// 先尝试 getpwuid(getuid())，失败则回退到 $USER
// 环境变量。两者都取不到时返回空字符串。
inline std::string user_name() {
    auto* pw = getpwuid(getuid());
    if (pw) return pw->pw_name;
    auto* e = getenv("USER");
    return e ? std::string(e) : std::string();
}

// 返回系统主机名。
// 使用 gethostname()，缓冲区 256 字节。
inline std::string host_name() {
    std::array<char, 256> buf{};
    if (gethostname(buf.data(), buf.size()) == 0) {
        buf[buf.size() - 1] = '\0';
        return buf.data();
    }
    return {};
}

// 返回键为 key 的环境变量的值。
// 变量未设置时返回空字符串。
inline std::string env(const std::string& key) {
    auto* v = getenv(key.c_str());
    return v ? std::string(v) : std::string();
}

// 返回当前工作目录。
// 使用 getcwd()，缓冲区 4096 字节。
inline std::string cwd() {
    std::array<char, 4096> buf{};
    auto* p = getcwd(buf.data(), buf.size());
    return p ? std::string(p) : std::string();
}

// 执行一条 shell 命令并捕获其标准输出。
//
// 使用 popen() 的 "r" 模式；命令交给 /bin/sh -c 执行。
// 返回捕获到的输出字符串（若末尾带换行则一并保留）。
// popen 失败或没有任何输出时返回空字符串。
inline std::string exec(const std::string& command) {
    std::string result;
    auto* fp = popen(command.c_str(), "r");
    if (!fp) return result;
    std::array<char, 4096> buf{};
    while (fgets(buf.data(), buf.size(), fp) != nullptr)
        result += buf.data();
    pclose(fp);
    return result;
}

} // namespace sys
} // namespace vaterm

#endif // VATERM_SYSTEM_HPP
