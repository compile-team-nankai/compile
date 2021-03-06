cmake_minimum_required(VERSION 3.20)
project(Compile CXX C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_CXX_STANDARD 11)

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/l.c ${CMAKE_SOURCE_DIR}/y.c
        COMMAND flex -o l.c lexical.l
        COMMAND bison -ydvo y.c --debug translate.y
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(gencode y.c l.c ast.c symbol_table.c main.c ast_symbol.c intermediate.cpp asm_generator.cpp)

add_custom_command(TARGET gencode
        POST_BUILD
        COMMAND rm l.c y.c y.h
        COMMAND cp ${CMAKE_BINARY_DIR}/gencode ${CMAKE_SOURCE_DIR}/gencode
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
