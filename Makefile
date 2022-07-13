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

# Default installation folder. Override with "make DESTDIR=/foo/bar"
DESTDIR = /tmp/mc1-sdk

# Host command configuration.
CP               = cp -a
RM               = rm -f
MKDIR            = mkdir -p
RMDIR            = rm -rf
CMAKE            = cmake
CMAKE_GENERATOR  =
MRISC32_GCC      = mrisc32-elf-gcc
MRISC32_AR       = mrisc32-elf-ar rcs

# Paths.
SDK_ROOT         = .
LIBMC1_DIR       = $(SDK_ROOT)/libmc1
LIBMC1           = $(LIBMC1_DIR)/out/libmc1.a
CRT0_DIR         = $(SDK_ROOT)/linker-scripts
LIBMC1CRT0       = $(CRT0_DIR)/build/libmc1crt0.a
TOOLS_DIR        = $(SDK_ROOT)/tools
PNG2MCI          = $(TOOLS_DIR)/png2mci
LINKERSCRIPT_DIR = $(SDK_ROOT)/linker-scripts


.PHONY: all install clean

all: $(LIBMC1) $(LIBMC1CRT0) $(PNG2MCI)

install: all
	@echo Installing to $(DESTDIR)...

	$(MKDIR) $(DESTDIR)/bin
	$(CP) $(PNG2MCI) $(DESTDIR)/bin/
	$(CP) $(TOOLS_DIR)/*.py $(DESTDIR)/bin/

	$(MKDIR) $(DESTDIR)/lib
	$(CP) $(LIBMC1) $(DESTDIR)/lib/
	$(CP) $(LIBMC1CRT0) $(DESTDIR)/lib/
	$(CP) $(LINKERSCRIPT_DIR)/*.ld  $(DESTDIR)/lib/

	$(MKDIR) $(DESTDIR)/include/mc1
	$(CP) $(LIBMC1_DIR)/include/mc1/* $(DESTDIR)/include/mc1/

clean:
	$(MAKE) -C $(LIBMC1_DIR) clean
	$(RMDIR) $(CRT0_DIR)/build
	$(RMDIR) $(TOOLS_DIR)/build
	$(RM) $(PNG2MCI)

$(LIBMC1CRT0): $(CRT0_DIR)/crt0.s
	$(MKDIR) $(CRT0_DIR)/build
	$(MRISC32_GCC) -c -I $(LIBMC1_DIR)/include -o $(CRT0_DIR)/build/crt0.o $(CRT0_DIR)/crt0.s
	$(MRISC32_AR) $(LIBMC1CRT0) $(CRT0_DIR)/build/crt0.o

$(LIBMC1):
	$(MAKE) -C $(LIBMC1_DIR)

$(PNG2MCI):
	$(MKDIR) $(TOOLS_DIR)/build
	$(CMAKE) $(CMAKE_GENERATOR) -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(TOOLS_DIR) -S $(TOOLS_DIR)/src -B $(TOOLS_DIR)/build
	$(CMAKE) --build $(TOOLS_DIR)/build
	DESTDIR= $(CMAKE) --install $(TOOLS_DIR)/build

