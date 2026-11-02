#!/bin/sh
# usage: addr2sym.sh <runtime-addr-of-function-F> <function-name-F> <runtime-addr>...   resolves runtime code addresses using build/raptor.elf
cd "$(dirname "$0")"; ref=$1; name=$2; shift 2
base=$(m68k-linux-gnu-nm build/raptor.elf | awk -v n="$name" '$3==n {print $1}')
for a in "$@"; do m68k-linux-gnu-addr2line -f -e build/raptor.elf $(printf '0x%x' $(( 0x$a - 0x$ref + 0x$base ))) | head -1; done
