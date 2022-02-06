# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-
#--------------------------------------------------------------------------------------------------
# Copyright (c) 2022 Marcus Geelnard
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

.PHONY: all clean

all: $(OUT)/demo2.elf

clean:
	rm -f $(OUT)/*
	make -C $(LIBMC1_DIR) clean

$(PNG2MCI):
	@echo "=============================================================================="
	@echo " Please build $(PNG2MCI) (see $(SDK_ROOT)/tools/README.md)"
	@echo "=============================================================================="
	@false

$(LIBMC1):
	make -C $(LIBMC1_DIR)

$(OUT)/crt0.o: $(SDK_ROOT)/linker-scripts/crt0.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/demo2.o: demo2.c
	$(CC) $(CFLAGS) -o $@ $<

$(OUT)/mc1-logo.o: mc1-logo.png $(PNG2MCI)
	$(PNG2MCI) --pal2 --nocomp $< $(OUT)/mc1-logo.mci
	$(RAW2C) $(OUT)/mc1-logo.mci mc1_logo_mci > $(OUT)/mc1-logo.c
	$(CC) $(CFLAGS) -o $@ $(OUT)/mc1-logo.c

$(OUT)/demo2.elf: $(OUT)/crt0.o $(OUT)/demo2.o $(OUT)/mc1-logo.o $(LIBMC1)
	$(LD) $(LDFLAGS_APP) -o $@ $(OUT)/crt0.o $(OUT)/demo2.o $(OUT)/mc1-logo.o -lmc1

