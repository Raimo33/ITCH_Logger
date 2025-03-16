#pragma once

#include <string_view>
#include <cstdint>

#include "macros.hpp"

namespace utils
{
  [[noreturn]] void throw_error(const std::string_view message);
  std::pair<std::string_view, std::string_view> split(const std::string_view string, const char delimiter);
  template<typename T> T inline min(const T a, const T b) { return (a < b) * a + (b < a) * b; }
  uint8_t ultoa(uint64_t value, char *buffer);
  uint16_t swap16(const uint16_t value);
  uint32_t swap32(const uint32_t value);
  uint64_t swap64(const uint64_t value);
}

#include "utils.inl"