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
CFLAGS       = -O3 -c -I $(SDK_ROOT)/libmc1/include
AS           = mrisc32-elf-as
ASFLAGS      = -I $(SDK_ROOT)/libmc1/include
LD           = mrisc32-elf-gcc
LDFLAGS_APP  = -T $(SDK_ROOT)/linker-scripts/app-vram.ld -mno-crt0 -L $(LIBMC1_DIR)/out

PNG2MCI      = $(SDK_ROOT)/tools/png2mci
RAW2C        = $(SDK_ROOT)/tools/raw2c.py

.PHONY: all clean FORCE

all: $(OUT)/demo2.elf

FORCE:

clean:
	rm -f $(OUT)/*
	$(MAKE) -C $(LIBMC1_DIR) clean

$(PNG2MCI):
	@echo "=============================================================================="
	@echo " Please build $(PNG2MCI) (see $(SDK_ROOT)/tools/README.md)"
	@echo "=============================================================================="
	@false

$(LIBMC1): FORCE
	$(MAKE) -C $(LIBMC1_DIR)

$(OUT)/crt0.o: $(SDK_ROOT)/crt0/crt0-app.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/demo2.o: demo2.c
	$(CC) $(CFLAGS) -o $@ $<

$(OUT)/is-that-a-supra.o: is-that-a-supra.png $(PNG2MCI)
	$(PNG2MCI) --pal4 --lzg $< $(OUT)/is-that-a-supra.mci
	$(RAW2C) $(OUT)/is-that-a-supra.mci pic_is_that_a_supra > $(OUT)/is-that-a-supra.c
	$(CC) $(CFLAGS) -o $@ $(OUT)/is-that-a-supra.c

$(OUT)/retrawave-car.o: retrawave-car.png $(PNG2MCI)
	$(PNG2MCI) --pal4 --lzg $< $(OUT)/retrawave-car.mci
	$(RAW2C) $(OUT)/retrawave-car.mci pic_retrawave_car > $(OUT)/retrawave-car.c
	$(CC) $(CFLAGS) -o $@ $(OUT)/retrawave-car.c

$(OUT)/phantom.o: phantom.png $(PNG2MCI)
	$(PNG2MCI) --pal4 --lzg $< $(OUT)/phantom.mci
	$(RAW2C) $(OUT)/phantom.mci pic_phantom > $(OUT)/phantom.c
	$(CC) $(CFLAGS) -o $@ $(OUT)/phantom.c

$(OUT)/ming-charset-32x32.o: ming-charset-32x32.png $(PNG2MCI)
	$(PNG2MCI) --pal4 $< $(OUT)/ming-charset-32x32.mci
	$(RAW2C) $(OUT)/ming-charset-32x32.mci font_32x32 > $(OUT)/ming-charset-32x32.c
	$(CC) $(CFLAGS) -o $@ $(OUT)/ming-charset-32x32.c

$(OUT)/demo2.elf: $(OUT)/crt0.o $(OUT)/demo2.o $(OUT)/is-that-a-supra.o $(OUT)/retrawave-car.o $(OUT)/phantom.o $(OUT)/ming-charset-32x32.o $(LIBMC1)
	$(LD) $(LDFLAGS_APP) -o $@ $(OUT)/crt0.o $(OUT)/demo2.o $(OUT)/is-that-a-supra.o $(OUT)/retrawave-car.o $(OUT)/phantom.o $(OUT)/ming-charset-32x32.o -lmc1

