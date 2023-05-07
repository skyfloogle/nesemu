cmake_minimum_required(VERSION 3.25)
project(nesemu)

set(CMAKE_CXX_STANDARD 17)

add_executable(nesemu src/main.cpp src/Cpu.cpp src/Cpu.h src/Ppu.cpp src/Ppu.h src/Ppu.h)
