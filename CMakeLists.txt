cmake_minimum_required(VERSION 3.21)
project(nesemu)

set(CMAKE_CXX_STANDARD 17)

add_executable(nesemu WIN32 source/main.cpp source/Cpu.cpp source/Cpu.h source/Ppu.cpp source/Ppu.h source/Ppu.h source/palette.h source/SdlPpu.cpp source/SdlPpu.h source/input.h)

target_link_libraries(nesemu citro2d citro3d)

#set(SDL2_DIR SDL2-2.26.5/cmake)

#find_package(SDL2 REQUIRED)
#include_directories(${SDL2_INCLUDE_DIRS})
#target_link_libraries(nesemu ${SDL2_LIBRARIES})