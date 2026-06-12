#ifndef VATERM_SYSTEM_HPP
#define VATERM_SYSTEM_HPP

#include <string>

namespace vaterm {

/// Retrieve system-level information.
///
/// All functions are stateless and thread-safe (they read from system APIs
/// or environment variables).
namespace sys {

/// Current user's login name.
std::string user_name();

/// Hostname of the machine.
std::string host_name();

/// Value of an environment variable, or empty string if not set.
std::string env(const std::string& key);

/// Absolute path of the current working directory.
std::string cwd();

/// Execute a command and return its stdout as a string.
/// Returns empty string on failure.
std::string exec(const std::string& command);

} // namespace sys
} // namespace vaterm

#endif // VATERM_SYSTEM_HPP

