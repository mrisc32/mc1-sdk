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

SDK_ROOT   = ../..
LIBMC1_DIR = $(SDK_ROOT)/libmc1
LIBMC1     = $(LIBMC1_DIR)/out/libmc1.a
OUT        = ./out

CPP        = mrisc32-elf-g++
CPPFLAGS   = -Os -std=c++17 -fno-exceptions -mcmodel=small -c -I $(SDK_ROOT)/libmc1/include -W -Wall -Wextra
AS         = mrisc32-elf-as
ASFLAGS    = -I $(SDK_ROOT)/libmc1/include
LD         = mrisc32-elf-gcc
LDFLAGS    = -T $(SDK_ROOT)/linker-scripts/boot-vram.ld -mno-crt0 -mno-ctor-dtor -L $(LIBMC1_DIR)/out

.PHONY: all clean FORCE

all: $(OUT)/shell.elf

clean:
	rm -f $(OUT)/*
	$(MAKE) -C $(LIBMC1_DIR) clean

FORCE:

$(LIBMC1): FORCE
	$(MAKE) -C $(LIBMC1_DIR)

$(OUT)/crt0.o: crt0.s
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/shell.o: shell.cpp
	$(CPP) $(CPPFLAGS) -o $@ $<

$(OUT)/shell.elf: $(OUT)/crt0.o $(OUT)/shell.o $(LIBMC1)
	$(LD) $(LDFLAGS) -o $@ $(OUT)/crt0.o $(OUT)/shell.o -lmc1

