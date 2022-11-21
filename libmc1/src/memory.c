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

#include <mc1/memory.h>
#include <mc1/mmio.h>

#include <stdint.h>

//--------------------------------------------------------------------------------------------------
// Private
//--------------------------------------------------------------------------------------------------

typedef struct {
  bool initialized;
  void* start;
  void* end;
  void* next_free;
} vmem_ctx_t;

typedef struct {
  void* block_start;
} block_meta_t;

static vmem_ctx_t s_vmem_ctx;

// Defined by the linker script.
extern int __vram_free_start;

static void* _vmem_align_4(void* ptr) {
  uintptr_t unaligned = ((uintptr_t)ptr) % 4U;
  if (unaligned) {
    return (void*)((uintptr_t)ptr + 4U - unaligned);
  }
  return ptr;
}

static void _vmem_init(void) {
  if (!s_vmem_ctx.initialized) {
    s_vmem_ctx.start = (void*)&__vram_free_start;
    s_vmem_ctx.end = (void*)(VRAM_START + MMIO(VRAMSIZE));
    s_vmem_ctx.next_free = s_vmem_ctx.start;
    s_vmem_ctx.initialized = true;
  }
}

//--------------------------------------------------------------------------------------------------
// Public
//--------------------------------------------------------------------------------------------------

void* vmem_alloc(size_t num_bytes) {
  _vmem_init();

  // Check if the allocation fits.
  void* meta_ptr = _vmem_align_4((void*)((uintptr_t)s_vmem_ctx.next_free + num_bytes));
  void* next_free = (void*)(sizeof(block_meta_t) + (uintptr_t)meta_ptr);
  if ((uintptr_t)next_free > (uintptr_t)s_vmem_ctx.end) {
    return NULL;
  }

  // Commit the allocation.
  void* ptr = s_vmem_ctx.next_free;
  ((block_meta_t*)meta_ptr)->block_start = ptr;
  s_vmem_ctx.next_free = next_free;

  return ptr;
}

bool vmem_free(void* ptr) {
  _vmem_init();

  // No allocations at all?
  if (s_vmem_ctx.next_free == s_vmem_ctx.start) {
    return false;
  }

  // Get the start of the most recently allocated block.
  void* meta_ptr = (void*)((uintptr_t)s_vmem_ctx.next_free - sizeof(block_meta_t));
  void* block_start = ((block_meta_t*)meta_ptr)->block_start;

  // Was the request for this block?
  if (block_start != ptr) {
    return false;
  }

  // Commit the de-allocation.
  s_vmem_ctx.next_free = block_start;

  return true;
}

size_t vmem_query_free(void) {
  _vmem_init();
  return (uintptr_t)s_vmem_ctx.end - (uintptr_t)s_vmem_ctx.next_free - sizeof(block_meta_t);
}
