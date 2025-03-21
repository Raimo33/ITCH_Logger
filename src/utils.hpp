#pragma once

#include <string_view>
#include <cstdint>

#include "macros.hpp"

namespace utils
{
  [[noreturn]] void throw_error(const std::string_view message);
  std::pair<std::string, std::string> split(const std::string_view string, const char delimiter);
  uint8_t ultoa(uint64_t value, char *buffer);
  template <typename T> inline T min(const T a, const T b);
}

#include "utils.inl"