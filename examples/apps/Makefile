# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-
#--------------------------------------------------------------------------------------------------
# Copyright (c) 2021 Marcus Geelnard
#
# This software is provided 'as-is', without any express or implied warranty. In no event will the
# authors be held liable for any damages arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose, including commercial
# applications, and to alter it and redistribute it freely, subject to the following restrictions:
#
#  1. The origin of this software must not be misrepresented; you must not claim that you wrote
#     the original software. If you use this software in a product, an acknowledgment in the
#     product documentation would be appreciated but is not required.
#
#  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
#     being the original software.
#
#  3. This notice may not be removed or altered from any source distribution.
#--------------------------------------------------------------------------------------------------

SDK_HOME = ../..

CC           = mrisc32-elf-gcc
CFLAGS       = -O2 -c -I $(SDK_HOME)/libmc1/include -I $(SDK_HOME)/boot/include
AS           = mrisc32-elf-as
ASFLAGS      = -I $(SDK_HOME)/libmc1/include
LD           = mrisc32-elf-gcc
LDFLAGS_BOOT = -T $(SDK_HOME)/boot/boot-block.ld -mno-crt0
LDFLAGS_APP  = -T $(SDK_HOME)/boot/app-vram.ld -mno-crt0
OBJCOPY      = mrisc32-elf-objcopy

OUT = ./out

.PHONY: clean all

all: $(OUT)/demo1.img

clean:
	rm -f $(OUT)/*

$(OUT)/boot-block.o: boot-block.c $(OUT)/demo1.raw
	$(CC) $(CFLAGS) -DAPP_SIZE=$(shell stat -L -c %s out/demo1.raw) -o $@ $<

$(OUT)/boot-block.elf: $(OUT)/boot-block.o
	$(LD) $(LDFLAGS_BOOT) -o $@ $<

$(OUT)/crt0.o: $(SDK_HOME)/boot/crt0.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/demo1.o: demo1.c
	$(CC) $(CFLAGS) -o $@ $<

$(OUT)/demo1.elf: $(OUT)/crt0.o $(OUT)/demo1.o
	$(LD) $(LDFLAGS_APP) -o $@ $(OUT)/crt0.o $(OUT)/demo1.o

$(OUT)/demo1.raw: $(OUT)/demo1.elf
	$(OBJCOPY) -O binary $(OUT)/demo1.elf $(OUT)/demo1.raw

$(OUT)/demo1.img: $(OUT)/boot-block.elf $(OUT)/demo1.raw
	$(OBJCOPY) -O binary $(OUT)/boot-block.elf $(OUT)/boot-block.raw
	$(SDK_HOME)/tools/mkmc1bootimage.py $(OUT)/boot-block.raw $@ $(OUT)/demo1.raw

