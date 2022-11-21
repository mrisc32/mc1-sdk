// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2019 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#ifndef MC1_MEMORY_H_
#define MC1_MEMORY_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fixed memory areas.
#define ROM_START  0x00000000
#define VRAM_START 0x40000000
#define XRAM_START 0x80000000

/// @brief Initialize the memory allocator.
void vmem_init(void);

/// @brief Allocate one continuous block of memory.
/// @param num_bytes Number of bytes to allocate.
/// @returns the address of the allocated block, or NULL if no memory could be
/// allocated.
/// @note The allocated block is guaranteed to be aligned on a 4-byte boundary.
void* vmem_alloc(size_t num_bytes);

/// @brief Free one block of memory.
/// @param ptr Pointer to the start of the memory block to free.
/// @returns true if the block could be free:d, otherwise false.
/// @note Memory must be free:d in the reverse order of allocation!
bool vmem_free(void* ptr);

/// @brief Query how much memory is free.
/// @returns the total number of bytes that are free for allocation.
size_t vmem_query_free(void);

#ifdef __cplusplus
}
#endif

#endif  // MC1_MEMORY_H_

