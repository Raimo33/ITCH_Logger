/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-14 19:09:39                                                 
last edited: 2025-03-29 14:49:19                                                

================================================================================*/

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <format>
#include <endian.h>

#include "Client.hpp"
#include "macros.hpp"
#include "error.hpp"

COLD Client::Client(const std::string_view ip, const uint16_t port) :
  bind_address{
    AF_INET,
    htons(port),
    { INADDR_ANY },
    {}
  },
  multicast_address{
    AF_INET,
    htons(port),
    { inet_addr(ip.data()) },
    {}
  },
  fd(createUdpSocket()),
  logger("itch_multicast")
{
  error |= (bind(fd, reinterpret_cast<const sockaddr *>(&bind_address), sizeof(bind_address)) == -1);

  ip_mreq mreq{};
  mreq.imr_interface.s_addr = bind_address.sin_addr.s_addr;
  mreq.imr_multiaddr.s_addr = multicast_address.sin_addr.s_addr;

  error |= (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1);

  CHECK_ERROR;
}

COLD Client::~Client(void)
{
  close(fd);
}

COLD int Client::createUdpSocket(void) const
{
  bool error = false;

  int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  error |= (sock_fd == -1);

  constexpr int enable = 1;
  constexpr int disable = 0;
  //constexpr int priority = 255;

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
  constexpr uint16_t MTU = 2000;
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

  while (true)
  {
    int8_t packets_count = recvmmsg(fd, packets, MAX_PACKETS, MSG_WAITALL, nullptr);
    error |= (packets_count == -1);

    const MoldUDP64Header *header_ptr = headers;
    const char *payload_ptr = reinterpret_cast<char *>(payloads);

    while (packets_count--)
    {
      PREFETCH_R(header_ptr + 1, 2);
      PREFETCH_R(payload_ptr + MAX_MSG_SIZE, 2);

      const uint16_t message_count = be16toh(header_ptr->message_count);
      processMessageBlocks(payload_ptr, message_count);

      header_ptr++;
      payload_ptr += MAX_MSG_SIZE;
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
    const uint16_t block_length = be16toh(block.length);

    PREFETCH_R(buffer + block_length + sizeof(block.length), 3);

    (this->*handlers[block.type])(block);

    buffer += block_length + sizeof(block.length);
  }
}

HOT void Client::handleNewOrder(const MessageBlock &block)
{
  const uint32_t timestamp = be32toh(block.new_order.timestamp_nanoseconds);
  const int32_t  price = be32toh(block.new_order.price);
  const uint64_t quantity = be64toh(block.new_order.quantity);
  const uint32_t orderbook_position = be32toh(block.new_order.orderbook_position);

  thread_local std::array<char, 256> buffer;
  const auto result = std::format_to_n(buffer.data(), buffer.size(),
    "{}, Timestamp: {}, Side: {}, Price: {}, Quantity: {}, Orderbook Position: {}\n",
    "NEW_ORDER", timestamp, block.new_order.side, price, quantity, orderbook_position);

  logger.log(std::string_view(buffer.data(), result.size));
}

HOT void Client::handleExecutionNotice(const MessageBlock &block)
{
  const uint32_t timestamp = be32toh(block.execution_notice.timestamp_nanoseconds);
  const uint32_t price = INT32_MAX;
  const uint64_t quantity = be64toh(block.execution_notice.executed_quantity);

  thread_local std::array<char, 256> buffer;
  const auto result = std::format_to_n(buffer.data(), buffer.size(),
    "{}, Timestamp: {}, Side: {}, Price: {}, Quantity: {}\n",
    "EXECUTION_NOTICE", timestamp, block.execution_notice.side, price, quantity);
  
  logger.log(std::string_view(buffer.data(), result.size)); 
}

HOT void Client::handleExecutionNoticeWithTradeInfo(const MessageBlock &block)
{
  const uint32_t timestamp = be32toh(block.execution_notice_with_trade_info.timestamp_nanoseconds);
  const int32_t  price = be32toh(block.execution_notice_with_trade_info.trade_price);
  const uint64_t quantity = be64toh(block.execution_notice_with_trade_info.executed_quantity);

  thread_local std::array<char, 256> buffer;
  const auto result = std::format_to_n(buffer.data(), buffer.size(),
    "{}, Timestamp: {}, Side: {}, Price: {}, Quantity: {}\n",
    "EXECUTION_NOTICE_WITH_TRADE_INFO", timestamp, block.execution_notice_with_trade_info.side, price, quantity);

  logger.log(std::string_view(buffer.data(), result.size));
}

HOT void Client::handleDeletedOrder(const MessageBlock &block)
{
  const uint32_t timestamp = be32toh(block.deleted_order.timestamp_nanoseconds);

  thread_local std::array<char, 256> buffer;
  const auto result = std::format_to_n(buffer.data(), buffer.size(),
    "{}, Timestamp: {}, Side: {}\n",
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
