#pragma once

#include <string>
#include <string_view>
#include <array>
#include <cstdint>
#include <sys/uio.h>
#include <liburing.h>
#include <memory>

constexpr size_t BUFFER_SIZE = 32 * 1024 * 1024;
constexpr uint16_t ALIGNMENT = 4096;

class Logger
{
  public:

    Logger(const std::string_view filename);
    ~Logger();

    void log(const std::string_view message);

  private:

    int create_file(void) const;
    void flush(void);

    const std::string filename;
    const int fd;
    char *buffers[2];
    uint8_t buf_idx;
    char *write_ptr;
    const char *end_ptr;
    io_uring ring;
};