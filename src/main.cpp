/*================================================================================

File: main.cpp                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-17 15:30:41                                                 
last edited: 2025-03-27 15:01:35                                                

================================================================================*/

#include <iostream>

#include "Client.hpp"
#include "error.hpp"

volatile bool error = false;

void init_signal_handler(void);

int main(int argc, char **argv)
{
  if (argc < 3)
   return 1;

  init_signal_handler();

  std::vector<std::string_view> args(argv + 1, argv + argc);

  Client client(args);
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