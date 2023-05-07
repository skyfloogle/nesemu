cmake_minimum_required(VERSION 3.25)
project(nesemu)

set(CMAKE_CXX_STANDARD 17)

add_executable(nesemu src/main.cpp src/Cpu.cpp src/Cpu.h src/Ppu.cpp src/Ppu.h src/Ppu.h src/palette.h src/SdlPpu.cpp src/SdlPpu.h)

set(SDL2_DIR SDL2-2.26.5/cmake)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(nesemu ${SDL2_LIBRARIES})