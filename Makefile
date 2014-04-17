
######################
# Makefile for Tinix #
######################


# Entry point of Tinix
# It must be as same as 'KernelEntryPointPhyAddr' in load.inc!!!
ENTRYPOINT	= 0x30400

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400


# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -f elf
CFLAGS		= -m32 -I include/ -c -fno-builtin
LDFLAGS		= -m elf_i386 -s -Ttext $(ENTRYPOINT)
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
TINIXBOOT	= boot/boot.bin boot/loader.bin
TINIXKERNEL	= kernel.bin
OBJS		= kernel/kernel.o kernel/syscall.o kernel/start.o kernel/main.o\
			kernel/clock.o kernel/i8259.o kernel/global.o kernel/protect.o\
			kernel/proc.o kernel/keyboard.o kernel/tty.o kernel/console.o kernel/systask.o \
			lib/klib.o lib/klibc.o lib/string.o lib/memstring.o lib/err.o lib/stdio.o lib/unistd.o\
			fs/main.o fs/opera.o fs/misc.o \
			driver/driver.o driver/hd.o
DASMOUTPUT	= kernel.bin.asm

# All Phony Targets
.PHONY : everything final image clean realclean disasm all buildimg

# Default starting position
everything : $(TINIXBOOT) $(TINIXKERNEL)

all : realclean everything

final : all clean

image : final buildimg

clean :
	rm -f $(OBJS)

realclean :
	rm -f $(OBJS) $(TINIXBOOT) $(TINIXKERNEL)

disasm :
	$(DASM) $(DASMFLAGS) $(TINIXKERNEL) > $(DASMOUTPUT)

# Write "boot.bin" & "loader.bin" into floppy image "TINIX.IMG"
# We assume that "TINIX.IMG" exists in current folder
buildimg :
	mount hyk.img /mnt/floppy -o loop
	cp -f boot/loader.bin /mnt/floppy/
	cp -f kernel.bin /mnt/floppy
	umount  /mnt/floppy

boot/boot.bin : boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/load.inc boot/include/fat12hdr.inc boot/include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

$(TINIXKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(TINIXKERNEL) $(OBJS)

kernel/kernel.o : kernel/kernel.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/syscall.o : kernel/syscall.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/start.o: kernel/start.c include/type.h include/const.h include/protect.h include/string.h include/proc.h include/proto.h \
			include/global.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/main.o: kernel/main.c include/type.h include/const.h include/protect.h include/string.h include/proc.h include/proto.h \
			include/global.h include/fs.h include/unistd.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/i8259.o: kernel/i8259.c include/type.h include/const.h include/protect.h include/string.h include/proc.h \
			include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/global.o: kernel/global.c include/type.h include/const.h include/protect.h include/proc.h \
			include/global.h include/proto.h include/systask.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/protect.o: kernel/protect.c include/type.h include/const.h include/protect.h include/proc.h include/proto.h \
			include/global.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/clock.o: kernel/clock.c include/type.h include/const.h include/protect.h include/string.h include/proc.h \
			include/global.h include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/proc.o: kernel/proc.c include/type.h include/const.h include/protect.h include/string.h include/proc.h include/proto.h \
			include/global.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/keyboard.o: kernel/keyboard.c include/type.h include/const.h include/protect.h include/proto.h include/string.h \
			include/proc.h include/global.h include/keyboard.h include/keymap.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/tty.o: kernel/tty.c include/type.h include/const.h include/protect.h include/proto.h include/string.h \
	include/console.h include/proc.h include/global.h include/keyboard.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/console.o: kernel/console.c include/type.h include/const.h \
	 include/proto.h
	$(CC) $(CFLAGS) -o $@ $<

kernel/systask.o: kernel/systask.c include/msg.h include/proc.h \
	 include/protect.h include/global.h include/tty.h include/console.h \
	  include/proc.h include/proto.h include/const.h include/err.h
	$(CC) $(CFLAGS) -o $@ $<

fs/main.o: fs/main.c include/msg.h include/type.h include/proc.h \
	 include/protect.h include/err.h include/nostdio.h include/drive.h include/config.h include/debug.h
	$(CC) $(CFLAGS) -o $@ $<

fs/opera.o: fs/opera.c include/fs.h include/type.h include/err.h \
	 include/string.h include/proc.h include/protect.h include/msg.h \
	  include/fs.h include/msg.h include/global.h include/tty.h \
	   include/console.h include/proto.h include/const.h include/hd.h include/math.h include/debug.h
	$(CC) $(CFLAGS) -o $@ $<

fs/misc.o: fs/misc.c include/fs.h include/type.h include/msg.h \
	 include/string.h
	$(CC) $(CFLAGS) -o $@ $<


lib/klibc.o: lib/klib.c include/type.h include/const.h include/protect.h include/string.h include/proc.h include/proto.h \
			include/global.h
	$(CC) $(CFLAGS) -o $@ $<

lib/klib.o : lib/klib.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/string.o : lib/string.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/err.o: lib/err.c include/const.h include/type.h include/tty.h \
	 include/err.h
	$(CC) $(CFLAGS) -o $@ $<


lib/stdio.o: lib/stdio.c include/type.h include/proc.h \
	 include/protect.h include/proto.h include/string.h include/const.h
	$(CC) $(CFLAGS) -o $@ $<

lib/unistd.o: lib/unistd.c include/msg.h include/type.h include/proc.h \
	 include/protect.h include/fs.h include/string.h  include/err.h
	$(CC) $(CFLAGS) -o $@ $<

lib/memstring.o: lib/memstring.c
	$(CC) $(CFLAGS) -o $@ $<

drive/driver.o: drive/driver.c include/drive.h include/proc.h include/protect.h \
	 include/type.h include/msg.h include/fs.h include/proc.h
	$(CC) $(CFLAGS) -o $@ $<

driver/hd.o: driver/hd.c include/type.h include/proc.h \
	 include/protect.h include/msg.h include/hd.h include/fs.h include/err.h  include/string.h\
	
	$(CC) $(CFLAGS) -o $@ $<
