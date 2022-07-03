// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2022 Marcus Geelnard
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

#include <stdbool.h>
#include <stdint.h>

#include <mc1/mmio.h>
#include <mc1/vconsole.h>

#include <mc1/newlib_integ.h>

extern char _end;
char* _getheapend(char*);

static inline uint32_t get_sp(void) {
  int32_t sp;
  __asm__ ("mov\t%0, sp" : "=r"(sp) : );
  return sp;
}

static inline uint32_t get_pc(void) {
  int32_t pc;
  __asm__ (
    "1:\n\t"
    "addpc\t%0, #1b@pc"
    : "=r"(pc)
    :
  );
  return pc;
}

static uint32_t memtest8(uint8_t* const mem, size_t nbytes) {
  uint32_t num_bad = 0U;
  for (int pass = 0; pass < 100; ++pass) {
    uint8_t x = pass;
    for (size_t i = 0; i < nbytes; ++i) {
      mem[i] = x;
      x = (x >> 3) ^ (x + 33) ^ (i << 4);
    }

    x = pass;
    for (size_t i = 0; i < nbytes; ++i) {
      uint8_t y = mem[i];
      if (y != x) {
        ++num_bad;
      }
      x = (x >> 3) ^ (x + 33) ^ (i << 4);
    }
  }

  return num_bad;
}

static uint32_t memtest32(uint32_t* const mem, size_t nwords) {
  uint32_t num_bad = 0U;
  for (int pass = 0; pass < 100; ++pass) {
    uint32_t x = pass;
    for (size_t i = 0; i < nwords; ++i) {
      mem[i] = x;
      x = (x >> 3) ^ (x + 33) ^ (i << 7);
    }

    x = pass;
    for (size_t i = 0; i < nwords; ++i) {
      uint32_t y = mem[i];
      if (y != x) {
        ++num_bad;
      }
      x = (x >> 3) ^ (x + 33) ^ (i << 7);
    }
  }

  return num_bad;
}

static void memtest(void* const start, size_t nbytes) {
  vcon_print("Memtest 0x");
  vcon_print_hex((uint32_t)start);
  vcon_print(", ");
  vcon_print_dec(nbytes);
  vcon_print(" bytes: ");

  bool ok = true;
  uint32_t num_bad;
  num_bad = memtest8((uint8_t*)start, nbytes);
  if (num_bad > 0U) {
    ok = false;
  }
  vcon_print_dec(num_bad);
  vcon_print(" bad (byte), ");

  num_bad = memtest32((uint32_t*)start, nbytes / 4);
  if (num_bad > 0U) {
    ok = false;
  }
  vcon_print_dec(num_bad);
  vcon_print(" bad (word)\n");
}

int main(void)
{
  mc1newlib_init(MC1NEWLIB_ALL);

  vcon_print("== MC1 System Test ==\n\n");

  {
    const uint32_t vram_size = MMIO(VRAMSIZE);
    vcon_print("VRAM: ");
    vcon_print_dec(vram_size);
    vcon_print(" bytes\n");
  }

  {
    const uint32_t xram_size = MMIO(XRAMSIZE);
    vcon_print("XRAM: ");
    vcon_print_dec(xram_size);
    vcon_print(" bytes\n");
  }

  {
    const uint32_t heap_start = (uint32_t)&_end;
    const uint32_t heap_end = _getheapend(heap_start);
    vcon_print("Heap: 0x");
    vcon_print_hex(heap_start);
    vcon_print(" - 0x");
    vcon_print_hex(heap_end);
    vcon_print("\n");
  }

  {
    const uint32_t sp = get_sp();
    vcon_print("SP:   0x");
    vcon_print_hex(sp);
    vcon_print("\n");
  }

  {
    const uint32_t pc = get_pc();
    vcon_print("PC:   0x");
    vcon_print_hex(pc);
    vcon_print("\n");
  }

  {
    // Select a good memory range for VRAM testing.
    // TODO(m): Be more clever about this.
    uint8_t* start = VRAM_START + MMIO(VRAMSIZE) - 10000;
    memtest(start, 4096);
  }

  {
    // Select a good memory range for XRAM testing.
    // TODO(m): Be more clever about this.
    uint8_t* start = XRAM_START + MMIO(XRAMSIZE) - 10000;
    memtest(start, 4096);
  }

  while (1);
}

