/*================================================================================

File: Logger.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-15 12:48:08                                                 
last edited: 2025-03-27 18:59:25                                                

================================================================================*/

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/uio.h>
#include <liburing.h>
#include <sys/mman.h>
#include <array>
#include <chrono>
#include <format>

#include "Logger.hpp"
#include "utils.hpp"
#include "error.hpp"

COLD Logger::Logger(const std::string_view filename) :
  filename(filename),
  fd(createFile(std::chrono::system_clock::now())),
  buffers{
    static_cast<char*>(operator new(WRITE_BUFFER_SIZE, std::align_val_t(ALIGNMENT))),
    static_cast<char*>(operator new(WRITE_BUFFER_SIZE, std::align_val_t(ALIGNMENT)))
  },
  buf_idx(0),
  write_ptr(buffers[buf_idx]),
  end_ptr(buffers[buf_idx] + WRITE_BUFFER_SIZE)
{
  error |= ((buffers[0] == nullptr) | (buffers[1] == nullptr));

  error |= (io_uring_queue_init(1, &ring, IORING_SETUP_SQPOLL) == -1);
  error |= (io_uring_register_files(&ring, &fd, 1) == -1);

  iovec iov[2] = {
    {buffers[0], WRITE_BUFFER_SIZE},
    {buffers[1], WRITE_BUFFER_SIZE}
  };

  error |= (io_uring_register_buffers(&ring, iov, 2) == -1);
  error |= (madvise(buffers[0], WRITE_BUFFER_SIZE, MADV_SEQUENTIAL) == -1);
  error |= (madvise(buffers[1], WRITE_BUFFER_SIZE, MADV_SEQUENTIAL) == -1);
  error |= (posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) == -1);

  CHECK_ERROR;
}

COLD int Logger::createFile(const std::chrono::system_clock::time_point &tp)
{
  std::string full_filename(this->filename);
  full_filename.append("_");
  full_filename.append(std::format("{:%Y-%m-%d}", tp));
  full_filename.append(".log");

  const int fd = open(full_filename.data(), O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT | O_NOATIME | O_LARGEFILE, 0666);
  error |= (fd == -1);

  CHECK_ERROR;

  return fd;
}

COLD Logger::~Logger()
{
  io_uring_unregister_files(&ring);
  io_uring_queue_exit(&ring);
  free(buffers[0]);
  free(buffers[1]);
  close(fd);
}

COLD void Logger::rotateFiles(void)
{
  close(fd);
  fd = createFile(std::chrono::system_clock::now());

  error |= posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
  error |= io_uring_register_files(&ring, &fd, 1);
  CHECK_ERROR;
}

HOT void Logger::log(const std::string_view message)
{
  const char *data = message.data();
  size_t remaining = message.size();

  while (remaining > 0)
  {
    const size_t to_copy = std::min<size_t>(remaining, end_ptr - write_ptr);
    std::memcpy(write_ptr, data, to_copy);
    write_ptr += to_copy;
    data += to_copy;
    remaining -= to_copy;

    if (UNLIKELY(write_ptr == end_ptr))
      flush();
  }
}

HOT void Logger::flush(void)
{
  io_uring_sqe *sqe = io_uring_get_sqe(&ring);
  error |= (sqe == nullptr);
  CHECK_ERROR;

  io_uring_prep_write_fixed(sqe, fd, buffers[buf_idx], WRITE_BUFFER_SIZE, -1, buf_idx);
  sqe->flags |= IOSQE_FIXED_FILE | IOSQE_CQE_SKIP_SUCCESS;

  error |= (io_uring_submit(&ring) == -1);

  buf_idx ^= 1;
  write_ptr = buffers[buf_idx];
  end_ptr = write_ptr + WRITE_BUFFER_SIZE;

  CHECK_ERROR;
}