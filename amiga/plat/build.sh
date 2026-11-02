#!/bin/sh
# usage: build.sh out.hunk objs...   (objs already compiled for 68020)
out=$1; shift
m68k-linux-gnu-ld -z noexecstack --no-warn-rwx-segments -T "$(dirname "$0")/link.ld" --emit-relocs -o "$out.elf" "$@" -L/usr/lib/gcc-cross/m68k-linux-gnu/13 -lgcc 2>&1 | grep -v "NOTE\|warning: .*note.GNU" 
"$(dirname "$0")/elf2hunk.py" "$out.elf" "$out"
