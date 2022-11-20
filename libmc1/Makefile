# -*- mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-
#--------------------------------------------------------------------------------------------------
# Copyright (c) 2019 Marcus Geelnard
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

OUT = out

# Default optimization flags (may be overridden by parent Makefiles).
CFLAGS_OPT = -Os

# TODO(m): Remove -Wno-array-bounds once the GCC 12 bug has been fixed upstream. This is a
# temporary workaround to make the MMIO() macro work (i.e. access a constant address).
# See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101379
CFLAGS_COMMON = -c $(CFLAGS_OPT) -ffast-math -I include \
                -Wall -Wextra -Wshadow -Wno-array-bounds -pedantic -Werror \
                -MMD -MP

CC       = mrisc32-elf-gcc
CCFLAGS  = $(CFLAGS_COMMON) -std=c11
CXX      = mrisc32-elf-g++
CXXFLAGS = $(CFLAGS_COMMON) -std=c++17 -Wold-style-cast -fno-exceptions
AS       = mrisc32-elf-gcc
ASFLAGS  = -c -I include
AR       = mrisc32-elf-ar
ARFLAGS  = rcs
OBJCOPY  = mrisc32-elf-objcopy
CP       = cp -a

.PHONY: clean all

all: $(OUT)/libmc1.a

clean:
	rm -f $(OUT)/*.a \
	      $(OUT)/*.c \
	      $(OUT)/*.d \
	      $(OUT)/*.s \
	      $(OUT)/*.o \
	      $(OUT)/*.raw

#-----------------------------------------------------------------------------
# libmc1.a - MC1 system library
#-----------------------------------------------------------------------------

LIBMC1_OBJS = \
    $(OUT)/crc7.o \
    $(OUT)/crc16.o \
    $(OUT)/crc32c.o \
    $(OUT)/elf32.o \
    $(OUT)/fast_math.o \
    $(OUT)/framebuffer.o \
    $(OUT)/gfx.o \
    $(OUT)/glyph_renderer.o \
    $(OUT)/keyboard.o \
    $(OUT)/keyboard_layout_en_us.o \
    $(OUT)/leds.o \
    $(OUT)/lzg_mc1.o \
    $(OUT)/mc1-font-8x8.o \
    $(OUT)/mci_decode.o \
    $(OUT)/memory.o \
    $(OUT)/mfat_mc1.o \
    $(OUT)/newlib_integ.o \
    $(OUT)/sdcard.o \
    $(OUT)/time.o \
    $(OUT)/vconsole.o \
    $(OUT)/vcp.o

$(OUT)/libmc1.a: $(LIBMC1_OBJS)
	$(AR) $(ARFLAGS) $@ $(LIBMC1_OBJS)

$(OUT)/%.o: src/%.c
	$(CC) $(CCFLAGS) -o $@ $<

$(OUT)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

ASINCS = \
    include/mc1/framebuffer.inc \
    include/mc1/memory.inc \
    include/mc1/mmio.inc

$(OUT)/%.o: src/%.s $(ASINCS)
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/%.o: $(OUT)/%.s $(ASINCS)
	$(AS) $(ASFLAGS) -o $@ $<

$(OUT)/mc1-font-8x8.s: src/mc1-font-8x8.raw
	# Trim out ASCII code points 32..127 (the printable ASCII chars).
	cat src/mc1-font-8x8.raw | \
	    tail -c +257  | \
	    head -c -1024 \
	    > $(OUT)/mc1-font-8x8-part.raw
	../tools/raw2asm.py --rev $(OUT)/mc1-font-8x8-part.raw mc1_font_8x8 > $@

# Include dependency files (generated when building the object files).
-include $(LIBMC1_OBJS:.o=.d)

