#ifndef RMS_UTILS_HPP
#define RMS_UTILS_HPP

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace rms {

[[nodiscard]] inline std::string_view trim_left(std::string_view s) {
  auto const pos = s.find_first_not_of(" \t\n\r\f\v");
  if (pos == std::string_view::npos) {
    return std::string_view{};
  }
  s.remove_prefix(pos);
  return s;
}

[[nodiscard]] inline std::string_view trim_right(std::string_view s) {
  auto const pos = s.find_last_not_of(" \t\n\r\f\v");
  if (pos == std::string_view::npos) {
    return std::string_view{};
  }
  s.remove_suffix(s.size() - pos - 1);
  return s;
}

[[nodiscard]] inline std::string_view trim(std::string_view s) {
  return trim_right(trim_left(s));
}

[[nodiscard]] inline std::optional<int> to_int(std::string_view sv) {
  int value = 0;
  auto const trimmed = trim(sv);
  if (trimmed.empty()) {
    return std::nullopt;
  }
  auto const [ptr, ec] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);
  if (ec == std::errc()) {
    return value;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<double> to_double(std::string_view sv) {
  auto const trimmed = trim(sv);
  if (trimmed.empty()) {
    return std::nullopt;
  }

  // Handle Fortran-style D exponents by copying into a temporary string.
  std::string tmp(trimmed);
  for (char &ch : tmp) {
    if (ch == 'D' || ch == 'd') {
      ch = 'E';
    }
  }

  char *end = nullptr;
  double const value = std::strtod(tmp.c_str(), &end);
  if (end != tmp.c_str()) {
    return value;
  }
  return std::nullopt;
}

template <typename F>
inline void for_each_token(std::string_view line, F &&fn) {
  std::size_t pos = 0;
  while (pos < line.size()) {
    auto const start = line.find_first_not_of(" \t\n\r\f\v", pos);
    if (start == std::string_view::npos) {
      return;
    }
    auto const end = line.find_first_of(" \t\n\r\f\v", start);
    auto const token = line.substr(start, end == std::string_view::npos ? line.size() - start : end - start);
    fn(token);
    if (end == std::string_view::npos) {
      return;
    }
    pos = end;
  }
}

} // namespace rms

#endif // RMS_UTILS_HPP
