/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-14 19:09:39                                                 
last edited: 2025-03-28 16:32:55                                                

================================================================================*/

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <array>
#include <sys/epoll.h>
#include <byteswap.h>

#include "Client.hpp"
#include "macros.hpp"
#include "error.hpp"

COLD Client::Client(const std::vector<std::string> &addresses) :
  sockets(createSockets(addresses)),
  epoll_fd(createEpollFd()),
  logger("itch_multicast") {}

COLD Client::~Client(void)
{
  for (const auto &socket : sockets)
    close(socket);
  close(epoll_fd);
}

COLD std::vector<int> Client::createSockets(const std::vector<std::string> &addresses) const
{
  std::vector<int> sockets;
  sockets.reserve(addresses.size());

  for (const auto &address : addresses)
  {
    const auto colon_pos = address.find(':');
    std::string ip = address.substr(0, colon_pos);
    std::string port = address.substr(colon_pos + 1);

    sockaddr_in bind_address{};
    bind_address.sin_family = AF_INET;
    bind_address.sin_port = htons(std::stoi(port));
    bind_address.sin_addr.s_addr = INADDR_ANY;

    sockaddr_in multicast_address{};
    multicast_address.sin_family = AF_INET;
    multicast_address.sin_port = htons(std::stoi(port));
    multicast_address.sin_addr.s_addr = inet_addr(ip.data());

    ip_mreq mreq{};
    mreq.imr_interface.s_addr = bind_address.sin_addr.s_addr;
    mreq.imr_multiaddr.s_addr = multicast_address.sin_addr.s_addr;

    const int sock_fd = createUdpSocket();
    error |= (bind(sock_fd, reinterpret_cast<const sockaddr *>(&bind_address), sizeof(bind_address)) == -1);
    error |= (setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1);

    sockets.push_back(sock_fd);
  }

  CHECK_ERROR;

  return sockets;
}

COLD int Client::createEpollFd(void) const
{
  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  error |= (epoll_fd == -1);

  for (const auto &socket : sockets)
  {
    epoll_event event{};
    event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
    event.data.fd = socket;

    error |= (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket, &event) == -1);
  }

  CHECK_ERROR;

  return epoll_fd;
}

COLD int Client::createUdpSocket(void) const
{
  bool error = false;

  int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  error |= (sock_fd == -1);

  constexpr int enable = 1;
  constexpr int disable = 0;
  //constexpr int priority = 255;

  //TODO add
  // error |= (setsockopt(sock_fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(enable)) == -1);
  // error |= (setsockopt(sock_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable)) == -1);
  error |= (setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof(disable)) == -1);
  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_BUSY_POLL, &enable, sizeof(enable)) == -1);

  //TODO remove
  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1);
  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) == -1);

  CHECK_ERROR;

  return sock_fd;
}

COLD void Client::run(void)
{
  constexpr uint8_t MAX_PACKETS = 64;
  constexpr uint16_t MTU = 1500;
  constexpr uint16_t MAX_MSG_SIZE = MTU - sizeof(MoldUDP64Header);

  //+1 added for safe prefetching past the last packet 
  alignas(64) mmsghdr packets[MAX_PACKETS + 1]{};
  alignas(64) iovec iov[MAX_PACKETS + 1][2]{};
  alignas(64) MoldUDP64Header headers[MAX_PACKETS + 1]{};
  alignas(64) char payloads[MAX_PACKETS + 1][MAX_MSG_SIZE]{};

  for (uint8_t i = 0; i < MAX_PACKETS; ++i)
  {
    iov[i][0] = { &headers[i], sizeof(headers[i]) };
    iov[i][1] = { payloads[i], sizeof(payloads[i]) };

    msghdr &msg_hdr = packets[i].msg_hdr;
    msg_hdr.msg_iov = iov[i];
    msg_hdr.msg_iovlen = 2;
  }

  std::vector<epoll_event> events(sockets.size());

  while (true)
  {
    int8_t n = epoll_wait(epoll_fd, events.data(), events.size(), -1);
    error |= (n == -1);

    while (n--)
    {
      const epoll_event &event = events[n];
      const int fd = event.data.fd;
      error |= (event.events & (EPOLLERR | EPOLLHUP));

      int8_t packets_count = recvmmsg(fd, packets, MAX_PACKETS, MSG_DONTWAIT, nullptr);
      error |= (packets_count == -1);

      const MoldUDP64Header *header_ptr = headers;
      const char *payload_ptr = reinterpret_cast<char *>(payloads);

      while (packets_count--)
      {
        PREFETCH_R(header_ptr + 1, 2);
        PREFETCH_R(payload_ptr + MAX_MSG_SIZE, 2);

        const uint16_t message_count = bswap_16(header_ptr->message_count);
        processMessageBlocks(payload_ptr, message_count);

        header_ptr++;
        payload_ptr += MAX_MSG_SIZE;
      }
    }
    CHECK_ERROR;
  }

  UNREACHABLE;
}

HOT void Client::processMessageBlocks(const char *buffer, uint16_t blocks_count)
{
  using MessageHandler = void (Client::*)(const MessageBlock &);

  constexpr uint8_t size = 'Z' + 1;
  alignas(64) constexpr std::array<MessageHandler, size> handlers = []()
  {
    std::array<MessageHandler, size> handlers{};
    handlers['A'] = &Client::handleNewOrder;
    handlers['D'] = &Client::handleDeletedOrder;
    handlers['T'] = &Client::handleSeconds;
    handlers['R'] = &Client::handleSeriesInfoBasic;
    handlers['M'] = &Client::handleSeriesInfoBasicCombination;
    handlers['L'] = &Client::handleTickSizeData;
    handlers['S'] = &Client::handleSystemEvent;
    handlers['O'] = &Client::handleTradingStatus;
    handlers['E'] = &Client::handleExecutionNotice;
    handlers['C'] = &Client::handleExecutionNoticeWithTradeInfo;
    handlers['Z'] = &Client::handleEquilibriumPrice;
    return handlers;
  }();

  while (blocks_count--)
  {
    const MessageBlock &block = *reinterpret_cast<const MessageBlock *>(buffer);
    const uint16_t block_length = bswap_16(block.length);

    PREFETCH_R(buffer + block_length + sizeof(block.length), 3);

    (this->*handlers[block.type])(block);

    buffer += block_length + sizeof(block.length);
  }
}

HOT void Client::handleNewOrder(const MessageBlock &block)
{
  const uint32_t timestamp = bswap_32(block.new_order.timestamp_nanoseconds);
  const int32_t  price = bswap_32(block.new_order.price);
  const uint64_t quantity = bswap_64(block.new_order.quantity);
  const uint32_t orderbook_position = bswap_32(block.new_order.orderbook_position);

  thread_local std::array<char, 256> buffer;
  const auto result = std::format_to_n(buffer.data(), buffer.size(),
    "{:<15}, Timestamp: {:>10}, Side: {}, Price: {:>10}, Quantity: {:>20}, Orderbook Position: {:>10}\n",
    "NEW_ORDER", timestamp, block.new_order.side, price, quantity, orderbook_position);

  logger.log(std::string_view(buffer.data(), result.size));
}

HOT void Client::handleExecutionNotice(const MessageBlock &block)
{
  const uint32_t timestamp = bswap_32(block.execution_notice.timestamp_nanoseconds);
  const uint32_t price = INT32_MAX;
  const uint64_t quantity = bswap_64(block.execution_notice.executed_quantity);

  thread_local std::array<char, 256> buffer;
  const auto result = std::format_to_n(buffer.data(), buffer.size(),
    "{:<15}, Timestamp: {:>10}, Side: {}, Price: {:>10}, Quantity: {:>20}\n",
    "EXECUTION_NOTICE", timestamp, block.execution_notice.side, price, quantity);
  
  logger.log(std::string_view(buffer.data(), result.size)); 
}

HOT void Client::handleExecutionNoticeWithTradeInfo(const MessageBlock &block)
{
  const uint32_t timestamp = bswap_32(block.execution_notice_with_trade_info.timestamp_nanoseconds);
  const int32_t  price = bswap_32(block.execution_notice_with_trade_info.trade_price);
  const uint64_t quantity = bswap_64(block.execution_notice_with_trade_info.executed_quantity);

  thread_local std::array<char, 256> buffer;
  const auto result = std::format_to_n(buffer.data(), buffer.size(),
    "{:<15}, Timestamp: {:>10}, Side: {}, Price: {:>10}, Quantity: {:>20}\n",
    "EXECUTION_NOTICE_WITH_TRADE_INFO", timestamp, block.execution_notice_with_trade_info.side, price, quantity);

  logger.log(std::string_view(buffer.data(), result.size));
}

HOT void Client::handleDeletedOrder(const MessageBlock &block)
{
  const uint32_t timestamp = bswap_32(block.deleted_order.timestamp_nanoseconds);

  thread_local std::array<char, 256> buffer;
  const auto result = std::format_to_n(buffer.data(), buffer.size(),
    "{:<15}, Timestamp: {:>10}, Side: {}\n",
    "DELETED_ORDER", timestamp, block.deleted_order.side);

  logger.log(std::string_view(buffer.data(), result.size));
}

void Client::handleSeconds(const MessageBlock &block)
{
  (void)block;
  return;
}

void Client::handleSeriesInfoBasic(const MessageBlock &block)
{
  (void)block;
  return;
}

void Client::handleSeriesInfoBasicCombination(const MessageBlock &block)
{
  (void)block;
  return;
}

void Client::handleTickSizeData(const MessageBlock &block)
{
  (void)block;
  return;
}

void Client::handleSystemEvent(const MessageBlock &block)
{
  switch (block.system_event_data.event_code)
  {
    case 'O':
      logger.rotateFiles();
      break;
    // case 'C':
      //stop
  }
}

void Client::handleTradingStatus(const MessageBlock &block)
{
  (void)block;
  return;
}

void Client::handleEquilibriumPrice(const MessageBlock &block)
{
  (void)block;
  return;
}
