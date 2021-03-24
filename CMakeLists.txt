cmake_minimum_required(VERSION 3.17)
project(simple_shell C)

set(CMAKE_C_STANDARD 99)

add_executable(simple_shell main.c alias.h alias.c display.c display.h history.h)