# -*- mode: CMake; tab-width: 2; indent-tabs-mode: nil; -*-
#---------------------------------------------------------------------------------------------------
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
#
#---------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------
# This is a CMake toolchain file for building MC1 applications that are loaded into VRAM.
#
# Usage:
#  cmake -DCMAKE_TOOLCHAIN_FILE=path/to/toolchain-mc1-app-vram.cmake <CMAKE OPTIONS>
#---------------------------------------------------------------------------------------------------

set(__MC1_CRT0_LIB "mc1crt0-app")
set(__MC1_LINKER_SCRIPT "app-vram.ld")

include("${CMAKE_CURRENT_LIST_DIR}/toolchain-mc1.cmake")

