#!/bin/bash

set -e

flex -o l.c lexical.l
bison -ydvo y.c --debug grammar.y
g++ y.c l.c ast.c symbol_table.c main.c ast_symbol.c intermediate.cpp -o exe
rm l.c y.c y.h
