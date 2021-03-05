cmake_minimum_required(VERSION 3.16)
project(lab3)

set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_FLAGS "-O0 -g -Wall -Wextra -Werror")

add_executable(lab2 src/picture.c
        src/utility.c src/task2.c)