cmake_minimum_required(VERSION 3.22)
project(memtest)
execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE OUTPUT_VARIABLE CACHELINE)
message(STATUS "CACHELINE=${CACHELINE}")
add_definitions(-DCLS=${CACHELINE})

add_executable(memtest memtest.c)