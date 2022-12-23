cmake_minimum_required(VERSION 3.24)
project(RefCountGC C)

set(CMAKE_C_STANDARD 11 -pthread)

add_executable(RefCountGC main.c ReferenceCounter.c ReferenceCounter.h)
