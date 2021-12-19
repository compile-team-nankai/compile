#!/bin/bash

./gencode $1 > tmp.asm
nasm -f elf tmp.asm -o tmp.o
gcc tmp.o -m32 -o $2
rm -f tmp.o tmp.asm
