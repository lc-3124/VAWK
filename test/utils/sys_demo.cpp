// -----------------------------------------------------------------------
//  sys_demo — demonstrates every function in vaterm::sys
//
//  Exercises:
//    sys::user_name()    — login name of the current user
//    sys::host_name()    — system hostname
//    sys::env(key)       — environment variable lookup
//    sys::cwd()          — current working directory
//    sys::exec(cmd)      — shell command execution via popen
//
//  Each call is followed by a PASS/FAIL assertion.
//
//  Run:  ./sys_demo
// -----------------------------------------------------------------------

#include <vaterm.hpp>
#include <cstdio>
#include <string>

using namespace vaterm;

// Helper: print a labelled value.
static void check(const std::string& label, const std::string& value) {
    printf("  %-15s = \"%s\"\n", label.c_str(), value.c_str());
}

int main() {
    printf("%s=== vaterm system API Demo ===%s\n\n",
           color::effect(TextEffect::BOLD).c_str(), color::reset().c_str());

    // 1. user_name()
    printf("1. user_name()\n");
    auto u = sys::user_name();
    check("user_name", u);
    bool user_ok = !u.empty();
    printf("  %s\n", user_ok ? "  PASS" : "  FAIL (empty)");

    // 2. host_name()
    printf("\n2. host_name()\n");
    auto h = sys::host_name();
    check("host_name", h);
    bool host_ok = !h.empty();
    printf("  %s\n", host_ok ? "  PASS" : "  FAIL (empty)");

    // 3. env(key)
    printf("\n3. env(key)\n");
    auto home = sys::env("HOME");
    check("env(\"HOME\")", home);
    auto shell = sys::env("SHELL");
    check("env(\"SHELL\")", shell);
    auto nonexist = sys::env("NONEXISTENT_VAR_12345");
    check("env(\"NONEXISTENT\")", nonexist.empty() ? "(empty)" : nonexist);
    printf("  %s\n", !home.empty() && nonexist.empty() ? "  PASS" : "  FAIL");

    // 4. cwd()
    printf("\n4. cwd()\n");
    auto c = sys::cwd();
    check("cwd", c);
    bool cwd_ok = !c.empty();
    printf("  %s\n", cwd_ok ? "  PASS" : "  FAIL (empty)");

    // 5. exec()
    printf("\n5. exec()\n");
    auto out = sys::exec("echo 'hello from exec'");
    out.pop_back();  // remove trailing newline from echo
    check("exec(\"echo...\")", out);
    bool exec_ok = (out == "hello from exec");
    printf("  %s\n", exec_ok ? "  PASS" : "  FAIL");

    auto fail = sys::exec("nonexistent_command_xyz 2>/dev/null");
    check("exec(\"invalid\")", fail.empty() ? "(empty)" : fail);
    printf("  %s\n", fail.empty() ? "  PASS (returns empty on failure)" : "  UNEXPECTED");

    // Summary.
    printf("\n%sAll system API tests completed.%s\n",
           color::fg(Color4::GREEN).c_str(), color::reset().c_str());

    int passed = user_ok + host_ok + cwd_ok + exec_ok;
    printf("%d/4 core API tests passed.\n", passed);
    return passed == 4 ? 0 : 1;
}
