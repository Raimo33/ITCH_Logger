/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-14 19:09:39                                                 
last edited: 2025-03-16 11:40:26                                                

================================================================================*/

#include "Client.hpp"
#include "utils.hpp"
#include "macros.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>

COLD Client::Client(const std::string_view bind_address_str, const std::string_view multicast_address_str) :
  multicast_address(create_address(multicast_address_str)),
  bind_address(create_address(bind_address_str)),
  mreq(create_mreq()),
  fd(create_udp_socket()),
  logger("itch_multicast.log")
{
  bind(fd, reinterpret_cast<const sockaddr *>(&bind_address), sizeof(bind_address));
  connect(fd, reinterpret_cast<const sockaddr *>(&multicast_address), sizeof(multicast_address));
}

COLD Client::~Client(void)
{
  close(fd);
}

COLD sockaddr_in Client::create_address(const std::string_view address_string) const
{
  const std::pair<std::string_view, std::string_view> address_parts = utils::split(address_string, ':');

  std::string_view ip = address_parts.first;
  std::string_view port = address_parts.second;

  const bool error = (ip.empty() | port.empty());
  if (error)
    utils::throw_error("Invalid address string");
  
  sockaddr_in address{};

  address.sin_family = AF_INET;
  htons(address.sin_port = std::stoi(port.data()));
  if (inet_pton(AF_INET, ip.data(), &address.sin_addr) != 1)
    utils::throw_error("Invalid IP address");

  return address;
}

COLD ip_mreq Client::create_mreq(void) const
{
  ip_mreq mreq{};

  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  mreq.imr_multiaddr = bind_address.sin_addr;

  return mreq;
}

COLD int Client::create_udp_socket(void) const
{
  bool error = false;

  int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  error |= (sock_fd == -1);

  constexpr int enable = 1;
  error |= (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1);
  error |= (setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1);

  if (error)
    utils::throw_error("Failed to create socket");

  return sock_fd;
}

void Client::run(void)
{
  char buffer[READ_BUFFER_SIZE];
  size_t buffer_filled = 0;
  size_t buffer_position = 0;

  while (true)
  {
    ssize_t bytes_received = recv(fd, buffer + buffer_filled, READ_BUFFER_SIZE - buffer_filled, 0);

    if (UNLIKELY(bytes_received <= 0))
      utils::throw_error("Failed to receive data");

    buffer_filled += bytes_received;

    while (buffer_position + sizeof(MoldUDP64Header) <= buffer_filled)
    {
      const MoldUDP64Header *header = reinterpret_cast<MoldUDP64Header *>(buffer + buffer_position);
      uint16_t message_count = utils::swap16(header->message_count);
      buffer_position += sizeof(MoldUDP64Header);

      while(message_count--)
      {
        constexpr uint8_t header_length = sizeof(MessageBlock::length) + sizeof(MessageBlock::type);
        if (UNLIKELY(buffer_position + header_length > buffer_filled))
          goto need_more_data;

        const MessageBlock *block = reinterpret_cast<MessageBlock *>(buffer + buffer_position);

        const uint16_t block_length = utils::swap16(block->length) - sizeof(MessageBlock::type);
        buffer_position += sizeof(MessageBlock::length);

        const char message_type = block->type;
        buffer_position += sizeof(MessageBlock::type);

        if (UNLIKELY(buffer_position + block_length > buffer_filled))
        {
          buffer_position -= header_length;
          goto need_more_data;
        }

        buffer_position += block_length;

        switch (message_type)
        {
          case 'A':
            handle_new_order(block->data.new_order);
            break;
          case 'E':
            handle_execution_notice(block->data.execution_notice);
            break;
          case 'C':
            handle_execution_notice_with_trade_info(block->data.execution_notice_with_trade_info);
            break;
          case 'D':
            handle_order_delete(block->data.order_delete);
            break;
        }
      }
    }

  need_more_data:
    memmove(buffer, buffer + buffer_position, buffer_filled - buffer_position);
    buffer_filled -= buffer_position;
    buffer_position = 0;
  }
}

void Client::handle_new_order(const MessageBlock::NewOrder &message_data)
{
  char buffer[] = "[New Order] Timestamp:            Side:   Price:             Quantity:                      Orderbook Position:           \n";
  constexpr uint16_t buffer_len = sizeof(buffer) - 1;

  constexpr uint16_t timestamp_offset = strlen("[New Order] Timestamp: ");
  constexpr uint16_t side_offset = timestamp_offset + strlen("           Side: ");
  constexpr uint16_t price_offset = side_offset + strlen("  Price: ");
  constexpr uint16_t quantity_offset = price_offset + strlen("             Quantity: ");
  constexpr uint16_t orderbook_position_offset = quantity_offset + strlen("                  Orderbook Position: ");

  const uint32_t  timestamp = utils::swap32(message_data.timestamp_nanoseconds);
  const int32_t   price = utils::swap32(message_data.price);
  const uint64_t  quantity = utils::swap64(message_data.quantity);
  const uint32_t  orderbook_position = utils::swap32(message_data.orderbook_position);

  buffer[side_offset] = message_data.side;

  //TODO all the copies

  logger.log(std::string_view(buffer, buffer_len));
}

void Client::handle_execution_notice(const MessageBlock::ExecutionNotice &message_data)
{
  char buffer[] = "[Execution Notice] Timestamp:            Side:   Quantity:                     \n";
  constexpr uint16_t buffer_len = sizeof(buffer) - 1;

  constexpr uint16_t timestamp_offset = strlen("[Execution Notice] Timestamp: ");
  constexpr uint16_t side_offset = timestamp_offset + strlen("           Side: ");
  constexpr uint16_t quantity_offset = side_offset + strlen("  Quantity: ");

  buffer[side_offset] = message_data.side;
  //TODO all the copies

  logger.log(std::string_view(buffer, buffer_len));
}

void Client::handle_execution_notice_with_trade_info(const MessageBlock::ExecutionNoticeWithTradeInfo &message_data)
{
  char buffer[] = "[Execution Notice With Trade Info] Timestamp:            Side:   Price:             Quantity:                     \n";
  constexpr uint16_t buffer_len = sizeof(buffer) - 1;

  constexpr uint16_t timestamp_offset = strlen("[Execution Notice With Trade Info] Timestamp: ");
  constexpr uint16_t side_offset = timestamp_offset + strlen("           Side: ");
  constexpr uint16_t price_offset = side_offset + strlen("  Price: ");
  constexpr uint16_t quantity_offset = price_offset + strlen("             Quantity: ");

  const uint32_t  timestamp = utils::swap32(message_data.timestamp_nanoseconds);
  const int32_t   price = utils::swap32(message_data.trade_price);
  const uint64_t  quantity = utils::swap64(message_data.executed_quantity);

  buffer[side_offset] = message_data.side;
  //TODO all the copies

  logger.log(std::string_view(buffer, buffer_len));
}

void Client::handle_order_delete(const MessageBlock::DeletedOrder &message_data)
{
  char buffer[] = "[Order Delete] Timestamp:            Side:   \n";
  constexpr uint16_t buffer_len = sizeof(buffer) - 1;

  constexpr uint16_t timestamp_offset = strlen("[Order Delete] Timestamp: ");
  constexpr uint16_t side_offset = timestamp_offset + strlen("           Side: ");

  buffer[side_offset] = message_data.side;
  //TODO all the copies

  logger.log(std::string_view(buffer, buffer_len));
}

