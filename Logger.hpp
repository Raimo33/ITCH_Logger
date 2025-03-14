#pragma once

#include <string>
#include <string_view>
#include <array>
#include <cstdint>
#include <sys/uio.h>
#include <memory>

#define BUFFER_SIZE 32 * 1024 * 1024
#define N_CHUNKS 8
#define ALIGNMENT 4096

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
    std::unique_ptr<char[]> buffer;
    char *write_ptr;
    const char *end_ptr; 
    const std::array<iovec, N_CHUNKS> iov;
};