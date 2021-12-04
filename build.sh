#!/bin/bash

set -e

flex -o l.c lexical.l
bison -ydvo y.c --debug grammar.y
gcc -c y.c l.c ast.c symbol_table.c main.c ast_symbol.c
g++ -c intermediate.cpp -std=c++11
g++ y.o l.o ast.o symbol_table.o main.o ast_symbol.o intermediate.o -o exe
rm l.c y.c y.h y.o l.o ast.o symbol_table.o main.o ast_symbol.o intermediate.o
