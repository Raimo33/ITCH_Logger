#pragma once

#include <string>
#include <string_view>
#include <array>
#include <cstdint>
#include <sys/uio.h>
#include <liburing.h>
#include <memory>

constexpr size_t WRITE_BUFFER_SIZE = 32 * 1024 * 1024;
constexpr uint16_t ALIGNMENT = 4096;

class Logger
{
  public:

    Logger(const std::string_view filename_template);
    ~Logger();

    void log(const std::string_view message);
    void rotateFiles(void);

  private:

    int createFile(const std::chrono::system_clock::time_point &tp);
    void flush(void);

    const std::string filename_template;
    std::array<int, 2> fds;
    std::array<char *, 2> buffers;
    uint8_t buf_idx;
    uint8_t fd_idx;
    char *write_ptr;
    const char *end_ptr;
    io_uring ring;
};