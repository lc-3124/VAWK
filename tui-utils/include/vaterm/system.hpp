#ifndef VATERM_SYSTEM_HPP
#define VATERM_SYSTEM_HPP

// -----------------------------------------------------------------------
//  vaterm::sys — system information helpers
//
//  A collection of POSIX wrappers that return basic OS-level information.
//  All functions are stateless and thread-safe (re-entrant).
//
//  Functions:
//    user_name()         — login name of the current user
//    host_name()         — system hostname (nodename)
//    env(key)            — value of an environment variable
//    cwd()               — current working directory
//    exec(command)       — run a command and capture its stdout
// -----------------------------------------------------------------------

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <pwd.h>

namespace vaterm {
namespace sys {

// Return the login name of the current user.
//
// First tries getpwuid(getuid()), falling back to the $USER
// environment variable. Returns an empty string if neither succeeds.
inline std::string user_name() {
    auto* pw = getpwuid(getuid());
    if (pw) return pw->pw_name;
    auto* e = getenv("USER");
    return e ? std::string(e) : std::string();
}

// Return the system hostname.
// Uses gethostname() with a 256-byte buffer.
inline std::string host_name() {
    std::array<char, 256> buf{};
    if (gethostname(buf.data(), buf.size()) == 0) {
        buf[buf.size() - 1] = '\0';
        return buf.data();
    }
    return {};
}

// Return the value of the environment variable identified by key.
// Returns an empty string if the variable is not set.
inline std::string env(const std::string& key) {
    auto* v = getenv(key.c_str());
    return v ? std::string(v) : std::string();
}

// Return the current working directory.
// Uses getcwd() with a 4096-byte buffer.
inline std::string cwd() {
    std::array<char, 4096> buf{};
    auto* p = getcwd(buf.data(), buf.size());
    return p ? std::string(p) : std::string();
}

// Execute a shell command and capture its standard output.
//
// Uses popen() with "r" mode. The command is passed to /bin/sh -c.
// Returns the captured output as a string (including trailing newlines
// if present). Returns an empty string if popen fails or produces no
// output.
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
