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

SDK_ROOT     = ../..
LIBMC1_DIR   = $(SDK_ROOT)/libmc1
LIBMC1       = $(LIBMC1_DIR)/out/libmc1.a
OUT          = ./out

CC           = mrisc32-elf-gcc
CFLAGS       = -O2 -c -I $(SDK_ROOT)/libmc1/include -I $(SDK_ROOT)/boot/include
AS           = mrisc32-elf-as
ASFLAGS      = -I $(SDK_ROOT)/libmc1/include
LD           = mrisc32-elf-gcc
LDFLAGS_BOOT = -T $(SDK_ROOT)/boot/boot-block.ld -mno-crt0
LDFLAGS_APP  = -T $(SDK_ROOT)/boot/app-vram.ld -mno-crt0 -L $(LIBMC1_DIR)/out
OBJCOPY      = mrisc32-elf-objcopy

.PHONY: clean all

all: $(OUT)/demo1.img

clean:
	rm -f $(OUT)/*
	make -C $(LIBMC1_DIR) clean

$(LIBMC1):
	make -C $(LIBMC1_DIR)

$(OUT)/boot-block.o: boot-block.c $(OUT)/demo1.raw
	$(CC) $(CFLAGS) -DAPP_SIZE=$(shell stat -L -c %s out/demo1.raw) -o $@ $<

$(OUT)/boot-block.elf: $(OUT)/boot-block.o
	$(LD) $(LDFLAGS_BOOT) -o $@ $<

$(OUT)/crt0.o: $(SDK_ROOT)/boot/crt0.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/demo1.o: demo1.c
	$(CC) $(CFLAGS) -o $@ $<

$(OUT)/retrawave-car.o: retrawave-car.png
	$(SDK_ROOT)/tools/png2mci --pal4 --lzg $< $(OUT)/retrawave-car.mci
	$(SDK_ROOT)/tools/raw2c.py $(OUT)/retrawave-car.mci picture > $(OUT)/retrawave-car.c
	$(CC) $(CFLAGS) -o $@ $(OUT)/retrawave-car.c

$(OUT)/ming-charset-32x32.o: ming-charset-32x32.png
	$(SDK_ROOT)/tools/png2mci --pal4 $< $(OUT)/ming-charset-32x32.mci
	$(SDK_ROOT)/tools/raw2c.py $(OUT)/ming-charset-32x32.mci font_32x32 > $(OUT)/ming-charset-32x32.c
	$(CC) $(CFLAGS) -o $@ $(OUT)/ming-charset-32x32.c

$(OUT)/demo1.elf: $(OUT)/crt0.o $(OUT)/demo1.o $(OUT)/retrawave-car.o $(OUT)/ming-charset-32x32.o $(LIBMC1)
	$(LD) $(LDFLAGS_APP) -o $@ $(OUT)/crt0.o $(OUT)/demo1.o $(OUT)/retrawave-car.o $(OUT)/ming-charset-32x32.o -lmc1

$(OUT)/demo1.raw: $(OUT)/demo1.elf
	$(OBJCOPY) -O binary $(OUT)/demo1.elf $(OUT)/demo1.raw

$(OUT)/demo1.img: $(OUT)/boot-block.elf $(OUT)/demo1.raw
	$(OBJCOPY) -O binary $(OUT)/boot-block.elf $(OUT)/boot-block.raw
	$(SDK_ROOT)/tools/mkmc1bootimage.py $(OUT)/boot-block.raw $@ $(OUT)/demo1.raw

