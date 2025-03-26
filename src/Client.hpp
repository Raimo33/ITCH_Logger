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
    
    sockaddr_in createAddress(const std::string_view address_string) const;
    ip_mreq createMreq(void) const;
    int createUdpSocket(void) const;

    void processMessageBlocks(const char *buffer, uint16_t blocks_count);

    void handleNewOrder(const MessageBlock &block);
    void handleExecutionNotice(const MessageBlock &block);
    void handleExecutionNoticeWithTradeInfo(const MessageBlock &block);
    void handleDeletedOrder(const MessageBlock &block);
    void handleSeconds(const MessageBlock &block);
    void handleSeriesInfoBasic(const MessageBlock &block);
    void handleSeriesInfoBasicCombination(const MessageBlock &block);
    void handleTickSizeData(const MessageBlock &block);
    void handleSystemEvent(const MessageBlock &block);
    void handleTradingStatus(const MessageBlock &block);
    void handleEquilibriumPrice(const MessageBlock &block);

    const sockaddr_in bind_address;
    const sockaddr_in multicast_address;
    const int fd;
    Logger logger;
};