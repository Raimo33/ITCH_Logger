#pragma once

#include <string>
#include <string_view>
#include <array>
#include <cstdint>
#include <sys/uio.h>
#include <liburing.h>
#include <memory>

constexpr size_t BUFFER_SIZE = 64 * 1024 * 1024;
constexpr uint8_t N_CHUNKS = 64;
constexpr uint16_t ALIGNMENT = 4096;
constexpr uint8_t QUEUE_DEPTH = 16;
constexpr size_t PREALLOCATED_FILE_SIZE = 365ULL * 24 * 60 * 60 * 100;

class Logger
{
  public:

    Logger(const std::string_view filename);
    ~Logger();

    void log(const std::string_view message);

  private:

    int create_file(void) const;
    const std::array<iovec, N_CHUNKS> precompute_iov(void);
    void flush(void);

    const std::string filename;
    const int fd;
    char *buffer;
    char *write_ptr;
    const char *end_ptr; 
    const std::array<iovec, N_CHUNKS> iov;
    io_uring ring;
};