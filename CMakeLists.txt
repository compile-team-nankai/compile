cmake_minimum_required(VERSION 3.20)
project(Compile)

set(CMAKE_C_STANDARD 99)

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/l.c ${CMAKE_SOURCE_DIR}/y.c
        COMMAND flex -o l.c lexical.l
        COMMAND bison -ydvo y.c --debug grammar.y
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(exe y.c l.c ast.c symbol_table.c main.c ast_symbol.c)

add_custom_command(TARGET exe
        POST_BUILD
        COMMAND rm l.c y.c y.h
        COMMAND cp ${CMAKE_BINARY_DIR}/exe ${CMAKE_SOURCE_DIR}/exe
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
