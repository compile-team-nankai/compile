#!/bin/bash

set -e

flex -o l.c lexical.l
bison -ydvo y.c --debug grammar.y
gcc y.c l.c ast.c symbol_table.c main.c ast_symbol.c -o exe
rm l.c y.c y.h
