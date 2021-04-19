// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2021 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented// you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// This is MC1 boot block program loads and runs an application.
//--------------------------------------------------------------------------------------------------

#include <boot/mc1-boot.h>

// Application start address.
#ifndef APP_ADDR
#define APP_ADDR 0x40000100
#endif

// Application location in the boot image.
#ifndef APP_SIZE
#error "Please define APP_SIZE!"
#endif
#define APP_FIRST_BLOCK 1
#define APP_NUM_BLOCKS (APP_SIZE+511)/512

typedef void _Noreturn (*start_fun_t)(void);

void MC1_BOOT_FUNCTION _boot(const void* rom_base) {
  // Load the application into RAM.
  blk_read(rom_base, (char*)APP_ADDR, 0, APP_FIRST_BLOCK, APP_NUM_BLOCKS);

  // Start the program.
  start_fun_t start = (start_fun_t)APP_ADDR;
  start();
}

