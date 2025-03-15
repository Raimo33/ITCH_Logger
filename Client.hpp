#pragma once

#include <string_view>
#include <netinet/in.h>

#include "Packets.hpp"
#include "Logger.hpp"

constexpr uint32_t READ_BUFFER_SIZE = 1024 * 1024;

class Client
{
  public:
    Client(const std::string_view bind_address, const std::string_view multicast_address);
    ~Client(void);

    void run(void);

  private:
    
    sockaddr_in create_address(const std::string_view address_string) const;
    ip_mreq create_mreq(void) const;
    int create_udp_socket(void) const;

    void handle_new_order(const MessageBlock::NewOrder &message_data);
    void handle_execution_notice(const MessageBlock::ExecutionNotice &message_data);
    void handle_execution_notice_with_trade_info(const MessageBlock::ExecutionNoticeWithTradeInfo &message_data);
    void handle_order_delete(const MessageBlock::DeletedOrder &message_data);

    const sockaddr_in multicast_address;
    const sockaddr_in bind_address;
    const ip_mreq mreq;
    const int fd;
    Logger logger;
};