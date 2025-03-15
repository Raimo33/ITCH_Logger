#include "Logger.hpp"
#include "utils.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/uio.h>
#include <liburing.h>
#include <sys/mman.h>

Logger::Logger(const std::string_view filename) :
  filename(filename),
  fd(create_file()),
  buffer(static_cast<char *>(aligned_alloc(ALIGNMENT, BUFFER_SIZE))),
  write_ptr(buffer),
  end_ptr(buffer + BUFFER_SIZE),
  iov(precompute_iov())
{
  bool error = false;

  error |= (fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, PREALLOCATED_FILE_SIZE) == -1);
  error |= (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) == -1);
  error |= (io_uring_register_files(&ring, &fd, 1) == -1);
  error |= (io_uring_register_buffers(&ring, iov.data(), N_CHUNKS) == -1);
  error |= (madvise(buffer, BUFFER_SIZE, MADV_SEQUENTIAL) == -1);
  error |= (posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) == -1);

  if (error)
    utils::throw_error("Failed to initialize logger");
}

int Logger::create_file(void) const
{
  const int fd = open(filename.data(), O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT | O_NOATIME | O_LARGEFILE, 0666);

  if (fd == -1)
    utils::throw_error("Failed to open log file");

  return fd;
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
  free(buffer);
  io_uring_unregister_files(&ring);
  io_uring_queue_exit(&ring);
  close(fd);
}

void Logger::log(const std::string_view message)
{
  const char *data = message.data();
  size_t remaining = message.size();

  while (remaining > 0)
  {
    size_t to_copy = utils::min<size_t>(remaining, end_ptr - write_ptr);
    std::memcpy(write_ptr, data, to_copy);
    write_ptr += to_copy;
    data += to_copy;
    remaining -= to_copy;

    if (write_ptr == end_ptr)
      flush();
  }
}

void Logger::flush(void)
{
  io_uring_sqe *sqe = io_uring_get_sqe(&ring);

  io_uring_prep_writev(sqe, fd, iov.data(), N_CHUNKS, -1);
  sqe->flags |= IOSQE_ASYNC | IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_CQE_SKIP_SUCCESS;
  if (io_uring_submit(&ring) < 0)
    utils::throw_error("Failed to submit writev operation");
  write_ptr = buffer;
}