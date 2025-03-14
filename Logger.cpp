#include "Logger.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/uio.h>

Logger::Logger(const std::string_view filename) :
  filename(filename),
  write_ptr(buffer),
  end_ptr(buffer + BUFFER_SIZE),
  iov(precompute_iov())
{
  fd = open(filename.data(), O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT | O_NOATIME | O_LARGEFILE, 0666);
}

const std::array<iovec, N_CHUNKS> Logger::precompute_iov(void)
{
  constexpr size_t chunk_size = BUFFER_SIZE / N_CHUNKS;
  std::array<iovec, N_CHUNKS> result = {};
  
  for (uint16_t i = 0; i < N_CHUNKS; ++i)
  {
    result[i].iov_base = buffer + (i * chunk_size);
    result[i].iov_len = chunk_size;
  }

  return result;
}

Logger::~Logger()
{
  flush();
  close(fd);
}

void Logger::log(const std::string_view message)
{
  const uint32_t message_size = message.size();

  if (write_ptr + message_size > end_ptr)
    flush();

  std::memcpy(write_ptr, message.data(), message_size);
  write_ptr += message_size;
}

void Logger::flush(void)
{
  writev(fd, iov.data(), N_CHUNKS);
  write_ptr = buffer;
}