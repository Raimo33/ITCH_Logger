/*================================================================================

File: utils.cpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-15 21:14:56                                                 
last edited: 2025-03-16 15:43:16                                                

================================================================================*/

#include "utils.hpp"

#include <string_view>
#include <stdexcept>
#include <cstdint>

namespace utils
{
  [[noreturn]] COLD NEVER_INLINE void throw_error(const std::string_view message)
  {
    throw std::runtime_error(std::string(message));
  }

  COLD std::pair<std::string_view, std::string_view> split(const std::string_view string, const char delimiter)
  {
    size_t pos = string.find(delimiter);

    if (pos != std::string_view::npos)
      return {string.substr(0, pos), string.substr(pos + 1)};

    return {string, ""};
  }

  HOT uint8_t ultoa(uint64_t num, char *buffer)
  {
    if (num == 0)
    {
      buffer[0] = '0';
      return 1;
    }
  
    const uint8_t digits = 1 +
      (num >= 10UL) +
      (num >= 100UL) +
      (num >= 1000UL) +
      (num >= 10000UL) +
      (num >= 100000UL) +
      (num >= 1000000UL) +
      (num >= 10000000UL) +
      (num >= 100000000UL) +
      (num >= 1000000000UL) +
      (num >= 10000000000UL) +
      (num >= 100000000000UL) +
      (num >= 1000000000000UL) +
      (num >= 10000000000000UL) +
      (num >= 100000000000000UL) +
      (num >= 1000000000000000UL) +
      (num >= 10000000000000000UL) +
      (num >= 100000000000000000UL) +
      (num >= 1000000000000000000UL);
  
    constexpr uint64_t power10[] = {
      1UL,
      10UL,
      100UL,
      1000UL,
      10000UL,
      100000UL,
      1000000UL,
      10000000UL,
      100000000UL,
      1000000000UL,
      10000000000UL,
      100000000000UL,
      1000000000000UL,
      10000000000000UL,
      100000000000000UL,
      1000000000000000UL,
      10000000000000000UL,
      100000000000000000UL,
      1000000000000000000UL,
    };
  
    uint64_t power = power10[digits - 1];
    char *p = buffer;
    uint64_t quotient;
    for (int i = 0; i < digits; i++)
    {
      quotient = num / power;
      *p++ = (char)('0' + quotient);
      num -= quotient * power;
      power /= 10;
    }
  
    return digits;
  }
}