/*================================================================================

File: error.cpp                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-21 20:36:24                                                 
last edited: 2025-03-21 20:36:24                                                

================================================================================*/

#include "error.hpp"
#include "macros.hpp"

[[noreturn]] COLD NEVER_INLINE void panic(void)
{
  throw std::runtime_error("Error occured, shit your pants");
}