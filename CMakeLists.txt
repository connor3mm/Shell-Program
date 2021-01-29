cmake_minimum_required(VERSION 3.17)
project(simple_shell C)

set(CMAKE_C_STANDARD 99)

add_executable(simple_shell main.c command.c command.h parser.c parser.h history.c history.h display.c display.h)