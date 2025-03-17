cmake_minimum_required(VERSION 3.5)
project(ITCH_Logger)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so)
set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")

add_executable(ITCH_Logger
  src/main.cpp
  src/Client.cpp
  src/Logger.cpp
  src/utils.cpp
)

target_compile_options(ITCH_Logger PRIVATE -Wall -Wextra -Werror -pedantic)
target_compile_options(ITCH_Logger PRIVATE -O3 -march=znver2 -flto)
target_link_libraries(ITCH_Logger PRIVATE uring)
