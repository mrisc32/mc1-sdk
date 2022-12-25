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
CRT0_DIR         = $(SDK_ROOT)/crt0
TOOLS_DIR        = $(SDK_ROOT)/tools
PNG2MCI          = $(TOOLS_DIR)/png2mci
LINKERSCRIPT_DIR = $(SDK_ROOT)/linker-scripts
SUPPORT_DIR      = $(SDK_ROOT)/support


.PHONY: all install clean libmc1 crt0 FORCE

all: libmc1 crt0 $(PNG2MCI)

FORCE:

install: all
	@echo Installing to $(DESTDIR)...

	$(MKDIR) $(DESTDIR)/bin
	$(CP) $(PNG2MCI) $(DESTDIR)/bin/
	$(CP) $(TOOLS_DIR)/*.py $(DESTDIR)/bin/

	$(MKDIR) $(DESTDIR)/lib
	$(CP) $(LIBMC1) $(DESTDIR)/lib/
	$(CP) $(CRT0_DIR)/out/*.a $(DESTDIR)/lib/
	$(CP) $(LINKERSCRIPT_DIR)/*.ld  $(DESTDIR)/lib/

	$(MKDIR) $(DESTDIR)/include/mc1
	$(CP) $(LIBMC1_DIR)/include/mc1/* $(DESTDIR)/include/mc1/

	$(MKDIR) $(DESTDIR)/share/cmake
	$(CP) $(SUPPORT_DIR)/cmake/* $(DESTDIR)/share/cmake/

clean:
	$(MAKE) -C $(LIBMC1_DIR) clean
	$(MAKE) -C $(CRT0_DIR) clean
	$(RMDIR) $(TOOLS_DIR)/out
	$(RM) $(PNG2MCI)

crt0:
	$(MAKE) -C $(CRT0_DIR)

libmc1:
	$(MAKE) -C $(LIBMC1_DIR)

$(PNG2MCI): FORCE
	$(MKDIR) $(TOOLS_DIR)/out
	$(CMAKE) $(CMAKE_GENERATOR) -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(TOOLS_DIR) -S $(TOOLS_DIR)/src -B $(TOOLS_DIR)/out
	$(CMAKE) --build $(TOOLS_DIR)/out
	DESTDIR= $(CMAKE) --install $(TOOLS_DIR)/out

