#ifndef VATERM_UTF_HPP
#define VATERM_UTF_HPP

#include <string>
#include <string_view>

namespace vaterm {

/// UTF-8 string utilities.
///
/// All functions are stateless and thread-safe.
namespace utf {

/// Return the number of code points (characters) in a UTF-8 string.
/// This is NOT the byte count.
size_t count(std::string_view s);

/// Return the byte length of the UTF-8 character at the start of `s`.
/// Returns 0 if `s` is empty or starts with an invalid lead byte.
size_t char_bytes(std::string_view s);

/// Extract the i-th code point (0-based) from a UTF-8 string.
/// Returns an empty string_view if i is out of range.
std::string_view at(std::string_view s, size_t i);

/// Return the display width of a single UTF-8 character (0 for control
/// chars, 2 for CJK, 1 otherwise).
int char_width(std::string_view s);

/// Return the total display width of a UTF-8 string.
int width(std::string_view s);

} // namespace utf
} // namespace vaterm

#endif // VATERM_UTF_HPP

