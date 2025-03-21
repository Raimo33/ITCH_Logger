/*================================================================================

File: main.cpp                                                                  
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-17 15:30:41                                                 
last edited: 2025-03-21 17:43:35                                                

================================================================================*/

#include "Client.hpp"
#include "ErrorHandler.hpp"

#include <iostream>

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " <bind_ip:bind_port> <multicast_ip:multicast_port>" << std::endl;
    return 1;
  }

  Client client(argv[1], argv[2]);
  client.run();
}