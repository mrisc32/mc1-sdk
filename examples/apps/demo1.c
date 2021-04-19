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

typedef struct {
  int r;
  int g;
  int b;
  int a;
} color_t;

static void wait_vbl(void) {
  const uint32_t old_frame_no = MMIO(VIDFRAMENO);
  while (MMIO(VIDFRAMENO) == old_frame_no);
}

static uint32_t to_abgr32(const color_t col) {
  return (((uint32_t)col.a) << 24) |
         (((uint32_t)col.b) << 16) |
         (((uint32_t)col.g) << 8) |
         (uint32_t)col.r;
}

int main(int argc, char** argv) {
  // Set the background color.
  volatile uint32_t* vram = (volatile uint32_t*)VRAM_START;
  vram[8] = 0x60000000;
  vram[10] = 0x50007fff;

  color_t col = {255, 128, 128, 128};
  int inc_r = 1;
  int inc_g = 2;
  int inc_b = 3;

  while (1) {
    vram[9] = to_abgr32(col);

    col.r += inc_r;
    if (col.r >= 255) {
      col.r = 255;
      inc_r = -inc_r;
    } else if (col.r <= 0) {
      col.r = 0;
      inc_r = -inc_r;
    }

    col.g += inc_g;
    if (col.g >= 255) {
      col.g = 255;
      inc_g = -inc_g;
    } else if (col.g <= 0) {
      col.g = 0;
      inc_g = -inc_g;
    }

    col.b += inc_b;
    if (col.b >= 255) {
      col.b = 255;
      inc_b = -inc_b;
    } else if (col.b <= 0) {
      col.b = 0;
      inc_b = -inc_b;
    }

    wait_vbl();
  }
}

