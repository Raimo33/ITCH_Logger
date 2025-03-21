/*================================================================================

File: error.hpp                                                                   
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-03-21 18:34:30                                                 
last edited: 2025-03-21 18:34:30                                                

================================================================================*/

#pragma once

#include <array>

#include "macros.hpp"

extern volatile bool error;

HOT ALWAYS_INLINE inline void ignore(void);
[[noreturn]] COLD NEVER_INLINE void panic(void);

static constexpr std::array<void (*)(), 2> funcs = { ignore, panic };

#define CHECK_ERROR funcs[error]()

#include "error.inl"