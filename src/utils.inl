#include "utils.hpp"

namespace utils
{
  template <typename T>
  inline T min(const T a, const T b)
  {
    return b ^ ((a ^ b) & -(a < b));
  }
}