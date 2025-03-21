/*================================================================================

File: Logger.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-15 12:48:08                                                 
last edited: 2025-03-21 20:36:24                                                

================================================================================*/

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/uio.h>
#include <liburing.h>
#include <sys/mman.h>

#include "Logger.hpp"
#include "utils.hpp"
#include "error.hpp"

Logger::Logger(const std::string_view filename) :
  filename(filename),
  fd(createFile()),
  buffers{
    static_cast<char *>(aligned_alloc(ALIGNMENT, WRITE_BUFFER_SIZE)),
    static_cast<char *>(aligned_alloc(ALIGNMENT, WRITE_BUFFER_SIZE))
  },
  buf_idx(0),
  write_ptr(buffers[buf_idx]),
  end_ptr(buffers[buf_idx] + WRITE_BUFFER_SIZE)
{
  error |= (io_uring_queue_init(1, &ring, IORING_SETUP_SQPOLL) == -1);
  error |= (io_uring_register_files(&ring, &fd, 1) == -1);
  iovec iov[2] = {{buffers[0], WRITE_BUFFER_SIZE}, {buffers[1], WRITE_BUFFER_SIZE}};
  error |= (io_uring_register_buffers(&ring, iov, 2) == -1);
  error |= (madvise(buffers[0], WRITE_BUFFER_SIZE, MADV_SEQUENTIAL) == -1);
  error |= (madvise(buffers[1], WRITE_BUFFER_SIZE, MADV_SEQUENTIAL) == -1);
  error |= (posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) == -1);

  CHECK_ERROR;
}

int Logger::createFile(void) const
{
  const int fd = open(filename.data(), O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT | O_NOATIME | O_LARGEFILE, 0666);
  error |= (fd == -1);

  CHECK_ERROR;

  return fd;
}

Logger::~Logger()
{
  io_uring_unregister_files(&ring);
  io_uring_queue_exit(&ring);
  free(buffers[0]);
  free(buffers[1]);
  close(fd);
}

void Logger::log(const std::string_view message)
{
  const char *data = message.data();
  size_t remaining = message.size();

  while (remaining > 0)
  {
    const size_t to_copy = utils::min<size_t>(remaining, end_ptr - write_ptr);
    std::memcpy(write_ptr, data, to_copy);
    write_ptr += to_copy;
    data += to_copy;
    remaining -= to_copy;

    if (UNLIKELY(write_ptr == end_ptr))
      flush();
  }
}

void Logger::flush(void)
{
  io_uring_sqe *sqe = io_uring_get_sqe(&ring);
  io_uring_prep_write_fixed(sqe, fd, buffers[buf_idx], WRITE_BUFFER_SIZE, -1, buf_idx);
  sqe->flags |= IOSQE_ASYNC | IOSQE_FIXED_FILE | IOSQE_IO_LINK | IOSQE_BUFFER_SELECT | IOSQE_CQE_SKIP_SUCCESS;

  buf_idx ^= 1;
  write_ptr = buffers[buf_idx];
  end_ptr = write_ptr + WRITE_BUFFER_SIZE;

  error |= (UNLIKELY(fallocate(fd, 0, lseek(fd, 0, SEEK_END), WRITE_BUFFER_SIZE) == -1));
  CHECK_ERROR;
}