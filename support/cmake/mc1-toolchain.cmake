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

set(MC1 TRUE)
set(MC1_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(MC1_SDK_LIB "${MC1_SDK_ROOT}/lib")
set(MC1_SDK_INCLUDE "${MC1_SDK_ROOT}/include")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mrisc32)

set(CMAKE_C_COMPILER mrisc32-elf-gcc)
set(CMAKE_CXX_COMPILER mrisc32-elf-g++)
set(CMAKE_OBJCOPY mrisc32-elf-objcopy)

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -x assembler-with-cpp")
set(CMAKE_C_FLAGS "-DMC1 -I${MC1_SDK_INCLUDE}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-DMC1 -I${MC1_SDK_INCLUDE}" CACHE STRING "" FORCE)

set(_mc1_exe_linker_flags "-L${MC1_SDK_LIB} -mno-crt0 -lmc1crt0-app -lmc1 -T app-xram.ld")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> -o <TARGET> <CMAKE_C_LINK_FLAGS> <LINK_LIBRARIES> <LINK_FLAGS> <OBJECTS> ${_mc1_exe_linker_flags}")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> -o <TARGET> <CMAKE_CXX_LINK_FLAGS> <LINK_LIBRARIES> <LINK_FLAGS> <OBJECTS> ${_mc1_exe_linker_flags}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

