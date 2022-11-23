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
CFLAGS       = -O3 -c -I $(SDK_ROOT)/libmc1/include -w -DMC1
AS           = mrisc32-elf-as
ASFLAGS      = -I $(SDK_ROOT)/libmc1/include
LD           = mrisc32-elf-gcc
LDFLAGS_APP  = -T $(SDK_ROOT)/linker-scripts/app-vram.ld -mno-crt0 -L $(LIBMC1_DIR)/out

.PHONY: all clean FORCE

all: $(OUT)/systest.elf

clean:
	rm -f $(OUT)/*
	$(MAKE) -C $(LIBMC1_DIR) clean

FORCE:

$(LIBMC1): FORCE
	$(MAKE) -C $(LIBMC1_DIR)

$(OUT)/crt0.o: $(SDK_ROOT)/crt0/crt0-app.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/systest.o: systest.c
	$(CC) $(CFLAGS) -o $@ $<

$(OUT)/systest.elf: $(OUT)/crt0.o $(OUT)/systest.o $(LIBMC1)
	$(LD) $(LDFLAGS_APP) -o $@ $(OUT)/crt0.o $(OUT)/systest.o -lmc1

