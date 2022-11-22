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

# Host command configuration.
RM          = rm -f
MRISC32_GCC = mrisc32-elf-gcc
MRISC32_AR  = mrisc32-elf-ar rcs

# Paths.
SDK_ROOT   = ..
LIBMC1_DIR = $(SDK_ROOT)/libmc1
OUT        = out

.PHONY: all clean

all: $(OUT)/libmc1crt0-app.a $(OUT)/libmc1crt0-boot.a

clean:
	rm -rf $(OUT)/*.o $(OUT)/*.a

$(OUT)/libmc1crt0-app.a: crt0-app.s
	$(MRISC32_GCC) -c -I $(LIBMC1_DIR)/include -o $(OUT)/crt0-app.o $<
	$(MRISC32_AR) $@ $(OUT)/crt0-app.o

$(OUT)/libmc1crt0-boot.a: crt0-boot.s
	$(MRISC32_GCC) -c -I $(LIBMC1_DIR)/include -o $(OUT)/crt0-boot.o $<
	$(MRISC32_AR) $@ $(OUT)/crt0-boot.o
