#include "utils.hpp"

#include <string_view>
#include <stdexcept>

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
}