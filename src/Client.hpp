#pragma once

#include <string_view>
#include <netinet/in.h>

#include "Packets.hpp"
#include "Logger.hpp"

constexpr uint32_t READ_BUFFER_SIZE = 1024 * 1024;

class Client
{
  public:
    Client(const std::vector<std::string> &addresses);
    ~Client(void);

    void run(void);

  private:

    std::vector<int> createSockets(const std::vector<std::string> &addresses) const;
    int createEpollFd(void) const;
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

    const std::vector<int> sockets;
    const int epoll_fd;
    Logger logger;
};