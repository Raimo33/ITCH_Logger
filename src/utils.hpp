#pragma once

#include <string_view>
#include <cstdint>

#include "macros.hpp"

namespace utils
{
  std::pair<std::string, std::string> split(const std::string_view string, const char delimiter);
  uint8_t ultoa(uint64_t value, char *buffer);
}