/*================================================================================

File: main.cpp                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-17 15:30:41                                                 
last edited: 2025-03-21 20:36:24                                                

================================================================================*/

#include <iostream>

#include "Client.hpp"
#include "error.hpp"

volatile bool error = false;

void init_signal_handler(void);

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <bind_ip:bind_port> <multicast_ip:multicast_port>" << std::endl;
    return 1;
  }

  init_signal_handler();

  Client client(argv[1], argv[2]);
  client.run();
}

void init_signal_handler(void)
{
  struct sigaction sa{};

  sa.sa_handler = [](int) { panic(); };

  error |= (sigaction(SIGINT, &sa, nullptr) == -1);
  error |= (sigaction(SIGTERM, &sa, nullptr) == -1);
  error |= (sigaction(SIGQUIT, &sa, nullptr) == -1);
  error |= (sigaction(SIGPIPE, &sa, nullptr) == -1);

  CHECK_ERROR;
}