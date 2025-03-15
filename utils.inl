#include "utils.hpp"

#include <cstdint>

namespace utils
{
  HOT inline uint16_t swap16(const uint16_t value)
  {
    return __builtin_bswap16(value);
  }

  HOT inline uint32_t swap32(const uint32_t value)
  {
    return __builtin_bswap32(value);
  }

  HOT inline uint64_t swap64(const uint64_t value)
  {
    return __builtin_bswap64(value);
  }
}