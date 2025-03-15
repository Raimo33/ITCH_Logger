cmake_minimum_required(VERSION 3.5)
project(ITCH_Logger)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ITCH_Logger
  main.cpp
  Client.cpp
  Logger.cpp
  utils.cpp
)

target_compile_options(ITCH_Logger PRIVATE -Wall -Wextra -Werror -pedantic)
target_compile_options(ITCH_Logger PRIVATE -O3 -march=native -flto)
target_link_libraries(ITCH_Logger PRIVATE uring)