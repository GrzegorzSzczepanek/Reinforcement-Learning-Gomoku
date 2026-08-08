cmake_minimum_required(VERSION 3.16)
project(RL_Gomoku CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# Wszystkie .cpp z src/ trafiają do builda automatycznie.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(gomoku ${SOURCES})
target_include_directories(gomoku PRIVATE "${CMAKE_SOURCE_DIR}/src")
