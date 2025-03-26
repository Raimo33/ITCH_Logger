#pragma once

#include <string>
#include <string_view>
#include <array>
#include <chrono>
#include <cstdint>
#include <sys/uio.h>
#include <liburing.h>
#include <memory>

constexpr size_t WRITE_BUFFER_SIZE = 1024;
constexpr uint16_t ALIGNMENT = 4096;

class Logger
{
  public:

    Logger(const std::string_view filename);
    ~Logger();

    void log(const std::string_view message);
    void rotateFiles(void);

  private:

    int createFile(const std::chrono::system_clock::time_point &tp);
    void flush(void);

    const std::string filename;
    int fd;
    std::array<char *, 2> buffers;
    uint8_t buf_idx;
    uint8_t fd_idx;
    char *write_ptr;
    const char *end_ptr;
    io_uring ring;
};