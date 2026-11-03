# Raptor Amiga build.  make -C amiga   (needs m68k-linux-gnu-gcc, python3)
PREFIX  := m68k-linux-gnu-
OPT     ?= -O2
CC      := $(PREFIX)gcc
SRC     := build/src
OBJ     := build/obj
CFLAGS  := -x c -m68030 -malign-int -funsigned-char -fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -D__NO_CTYPE $(OPT) -w -std=gnu89 \
           -fno-pic -fno-pie -ffreestanding -fno-builtin -Ishim -I$(SRC) -Iplat -D__cdecl= -D__far= -D__near= -D__interrupt= -DO_BINARY=0 -D_MAX_PATH=260 -include errno.h -DAMIGA $(EXTRA) -MMD -MP
GAME    := anims bonus enemy eshot flame fx help input intro loadsave movie objects shadows shots store tile windows rap \
           vmemapi prefapi ptrapi gfxapi exitapi glbapi imsapi kbdapi swdapi
PLAT    := audio inject libc video tsm dosemu asm stubs main glbswap
OBJS    := $(addprefix $(OBJ)/,$(addsuffix .o,$(GAME) $(PLAT))) $(OBJ)/c2p.o

export RAPTOR_DATA
all: build/raptor
$(SRC)/.stamp: mirror.sh
	./mirror.sh && touch $@
$(OBJ)/%.o: $(SRC)/%.c $(SRC)/.stamp
	@mkdir -p $(OBJ); $(CC) $(CFLAGS) -c $< -o $@
$(OBJ)/rap.o: $(SRC)/rap.c $(SRC)/.stamp
	@mkdir -p $(OBJ); $(CC) $(CFLAGS) -Dmain=raptor_main -c $< -o $@
$(OBJ)/%.o: plat/%.c
	@mkdir -p $(OBJ); $(CC) $(CFLAGS) -std=gnu99 -fno-tree-loop-distribute-patterns -c $< -o $@
$(OBJ)/%.o: plat/%.s
	@mkdir -p $(OBJ); $(CC) -m68020 -c $< -o $@
build/raptor: $(OBJ)/crt0.o $(OBJS)
	plat/build.sh $@ $^
-include $(OBJS:.o=.d)
clean:
	rm -rf build
.PHONY: all clean
