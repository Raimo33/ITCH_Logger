#include "Client.hpp"

#include <iostream>

int main(int argc, char **argv)
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: " << argv[0] << " <bind_ip:bind_port> <multicast_ip:multicast_port>" << std::endl;
      return 1;
    }
  
    Client client(argv[1], argv[2]);
    client.run();

    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}