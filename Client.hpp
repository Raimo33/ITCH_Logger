#pragma once

#include <string_view>
#include <netinet/in.h>

#include "Packets.hpp"
#include "Logger.hpp"

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

    const sockaddr_in multicast_address;
    const sockaddr_in bind_address;
    const ip_mreq mreq;
    const int fd;
    Logger logger;
};