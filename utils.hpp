#pragma once

#include <string_view>

#include "macros.hpp"

namespace utils
{
  [[noreturn]] void throw_error(const std::string_view message);
  std::pair<std::string_view, std::string_view> split(const std::string_view string, const char delimiter);
  template<typename T> T inline min(const T a, const T b) { return (a < b) * a + (b < a) * b; }
}