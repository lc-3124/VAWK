//- vaterm::sys — 实现

#include "vaterm/system.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <pwd.h>

namespace vaterm {
namespace sys {

std::string user_name() {
    auto* pw = getpwuid(getuid());
    if (pw) return pw->pw_name;
    auto* e = getenv("USER");
    return e ? std::string(e) : std::string();
}

std::string host_name() {
    std::array<char, 256> buf{};
    if (gethostname(buf.data(), buf.size()) == 0) {
        buf[buf.size() - 1] = '\0';
        return buf.data();
    }
    return {};
}

std::string env(const std::string& key) {
    auto* v = getenv(key.c_str());
    return v ? std::string(v) : std::string();
}

std::string cwd() {
    std::array<char, 4096> buf{};
    auto* p = getcwd(buf.data(), buf.size());
    return p ? std::string(p) : std::string();
}

std::string exec(const std::string& command) {
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
