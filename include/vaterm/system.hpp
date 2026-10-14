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
//
//  实现见 src/vaterm/system.cpp。
// -----------------------------------------------------------------------

#include <string>

namespace vaterm {
namespace sys {

// 返回当前用户的登录名。
//
// 先尝试 getpwuid(getuid())，失败则回退到 $USER
// 环境变量。两者都取不到时返回空字符串。
std::string user_name();

// 返回系统主机名。
// 使用 gethostname()，缓冲区 256 字节。
std::string host_name();

// 返回键为 key 的环境变量的值。
// 变量未设置时返回空字符串。
std::string env(const std::string& key);

// 返回当前工作目录。
// 使用 getcwd()，缓冲区 4096 字节。
std::string cwd();

// 执行一条 shell 命令并捕获其标准输出。
//
// 使用 popen() 的 "r" 模式；命令交给 /bin/sh -c 执行。
// 返回捕获到的输出字符串（若末尾带换行则一并保留）。
// popen 失败或没有任何输出时返回空字符串。
std::string exec(const std::string& command);

} // namespace sys
} // namespace vaterm

#endif // VATERM_SYSTEM_HPP
