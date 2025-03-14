/*================================================================================

File: Client.cpp                                                                
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-14 19:09:39                                                 
last edited: 2025-03-14 19:09:39                                                

================================================================================*/

#include "Client.hpp"
#include "utils.hpp"
#include "macros.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

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

  if (ip.empty() || port.empty())
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
  error |= setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  error |= setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

  if (error)
    utils::throw_error("Failed to create socket");

  return sock_fd;
}

HOT void Client::run(void)
{
  MoldUDP64Header header{};
  MessageBlock message{};

  while (true)
  {
    recv(fd, &header, sizeof(header), MSG_WAITALL);
    
    for (uint16_t i = 0; i < header.message_count; ++i)
    {
      recv(fd, &message, sizeof(message.length) + sizeof(message.type), MSG_WAITALL);

      const bool is_order = (message.type == 'A' | message.type == 'E' | message.type == 'C' | message.type == 'D');
      if (!is_order)
      {
        lseek(fd, message.length, SEEK_CUR);
        continue;
      }

      recv(fd, &message.data, message.length - sizeof(message.type), MSG_WAITALL);

      switch (message.type)
      {
        case 'A':
          logger.log("New order");
          break;
        case 'E':
          logger.log("Execution notice");
          break;
        case 'C':
          logger.log("Execution notice with trade info");
          break;
        case 'D':
          logger.log("Order delete");
          break;
      }
    }
  }
}