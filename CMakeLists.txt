cmake_minimum_required(VERSION 3.20)
project(ts C)

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Wextra -pedantic -Werror)

add_executable(ts main.c vendor/stretchy_buffer.h)
