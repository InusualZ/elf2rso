cmake_minimum_required(VERSION 3.20)
project(elf2rso)

set(CMAKE_CXX_STANDARD 17)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -Wall")

include_directories(.)
include_directories(elfio)

add_executable(elf2rso elf2rso.cpp FileWriter.h optparser.h swap.h types.h)
