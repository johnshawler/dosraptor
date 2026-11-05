# Raptor: Call of the Shadows – Amiga 1200 (AGA) port

A source port of the DOS v1.2 sources in this repository to the Amiga 1200 (target: 68EC030, AGA, 2 MB chip, 8 MB fast, Kickstart 3.1; all measurements use FS-UAE with `cpu = 68030`).
The game code (`SOURCE/`, `GFX/`) is compiled unchanged apart from small compatibility patches that `mirror.sh` applies to a
copy of the tree in `build/src`; everything platform specific lives in `plat/`.

Status: boots, plays the intro, menus, pilot creation, hangar, level loading and the first level (shareware data) with sound effects
(software mixer on Paula) and music (the OPL2/GENMIDI scores rendered offline by `tools/mus2pcm.py`).
**Not done yet / unverified:** saving and loading pilots, the credits/demo/help screens, a frame-by-frame comparison with the DOS
version, real hardware (everything was run in FS-UAE with `cpu = 68030`; the stock 68020 A1200 is not a target).

## Build
Needs `m68k-linux-gnu-gcc`, `binutils-m68k-linux-gnu`, `libc6-dev-m68k-cross` (headers only) and Python 3.

    make -C amiga RAPTOR_DATA=/path/to/the/shareware/raptor/directory      # -> amiga/build/raptor (AmigaOS hunk executable)

The game data is **not** part of the repository.  The shareware data numbers its items differently from the tables in the source, so
`mkinc.py` regenerates `FILE0000.INC`/`FILE0001.INC` from the `.GLB` files you supply (names are matched).

## Run
Copy `raptor`, `FILE0000.GLB`, `FILE0001.GLB` and a `SETUP.INI` (set `CardType=0`, `Volume=0`) into one directory of an Amiga volume
and start `raptor` from it, or use the FS-UAE helpers:

    ./run.sh 60              # boots in FS-UAE under Xvfb, screenshots every 5 s into /tmp/raptor-hd
    python3 drive.py script  # scripted run (wait=/sleep=/key=/shot=), optional INJECT=<file> for scripted game input (see plat/inject.c)

`KICKSTART` (a 3.1 A1200 ROM, not included) and `RAPTOR_DATA` select the ROM and data.  `in_fly.txt` is an input script that
plays through pilot creation to the first level.

## Layout of `plat/`
| file | purpose |
|---|---|
| `crt0.s`, `link.ld`, `elf2hunk.py`, `build.sh` | start-up, linking and ELF → AmigaOS hunk conversion |
| `libc.c`, `os.h` (`gen_os.py`) | freestanding C library over dos/exec; library call stubs |
| `video.c`, `c2p.s` | AGA 320x200x8 display with own Copper list, chunky-to-planar, palette, mouse/keyboard |
| `tsm.c` | timer services driven by the vertical-blank interrupt |
| `dosemu.c`, `../shim/dos.h` | DOS/BIOS/VGA/mouse/DPMI API emulation |
| `asm.c` | C versions of the 386 assembler routines |
| `glbswap.c` | little-endian → big-endian conversion of GLB items (PIC, BLK, FNT, SWD, AGX, MAP, ITM) |
| `inject.c` | scripted input for automated tests |
| `stubs.c` | sound hardware stubs |
