cmake_minimum_required(VERSION 3.20)
project(ts C)

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Wextra -pedantic -Werror)

add_executable(ts main.c bind.c bind.h lexer.c lexer.h parser.c parser.h vendor/stretchy_buffer.h ast.h ast.c)
