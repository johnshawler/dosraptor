#!/bin/sh
# syntax-check one mirrored source file for 68020
cd "$(dirname "$0")"; f=$1; shift
m68k-linux-gnu-gcc -x c -m68020 -fsyntax-only -w -DO_BINARY=0 -D_MAX_PATH=260 -include errno.h -D__cdecl= -D__far= -D__near= -D__interrupt=  -std=gnu89 -Ishim -Ibuild/src "$@" build/src/$f
