cmake_minimum_required(VERSION 3.16)
project(lab3)

set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_FLAGS "-O0 -g -Wall -Wextra -Werror")

add_executable(lab4 src/picture.c
        src/utility.c
        src/task4.c
        src/color_space.c)