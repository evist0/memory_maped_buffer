cmake_minimum_required(VERSION 3.16)
project(course_work)

set(CMAKE_CXX_STANDARD 17)

add_executable(course_work main.cpp
        src/buffer.cpp
        include/buffer.hpp)