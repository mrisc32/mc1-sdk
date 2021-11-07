// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
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

#include <mc1/gfx.h>

#include <algorithm>
#include <cstring>

#ifdef __MRISC32__
#include <mr32intrin.h>
#endif

namespace {

inline uint32_t repeat2x16(const uint32_t x) {
#ifdef __MRISC32__
  return _mr32_shuf(x, _MR32_SHUFCTL(0, 1, 0, 1, 0));
#else
  return (x << 16) | (x & 0xffff);
#endif
}

inline uint32_t repeat4x8(const uint32_t x) {
#ifdef __MRISC32__
  return _mr32_shuf(x, _MR32_SHUFCTL(0, 0, 0, 0, 0));
#else
  return repeat2x16((x << 8) | (x & 255));
#endif
}

inline uint32_t repeat8x4(const uint32_t x) {
#ifdef __MRISC32_PACKED_OPS__
  return repeat4x8(_mr32_pack_b(x, x));
#else
  return repeat4x8((x << 4) | (x & 15));
#endif
}

inline uint32_t repeat16x2(const uint32_t x) {
  return repeat8x4((x << 2) | (x & 3));
}

inline uint32_t repeat32x1(const uint32_t x) {
  return (static_cast<int32_t>(x) << 31) >> 31;
}

inline uint32_t bitmix(const uint32_t mask, const uint32_t a, const uint32_t b) {
#ifdef __MRISC32__
  uint32_t result = a;
  __asm volatile("sel.213 %[result],%[mask],%[b]"
                 : [ result ] "+r"(result)
                 : [ mask ] "r"(mask), [ b ] "r"(b));
  return result;
#else
  return (a & mask) | (b & ~mask);
#endif
}

// TODO(m): Specialize this template for LOG2PPW == 0 (i.e. no head/tail necessary).
template <uint32_t LOG2PPW>
void gfx_fill_rect_internal(fb_t* fb,
                            const int x0,
                            const int y0,
                            const int w,
                            const int h,
                            const uint32_t color) {
  constexpr auto PPW = 1 << LOG2PPW;
  constexpr auto BPP = 32 >> LOG2PPW;

  const uint32_t align = x0 & (PPW - 1);
  const uint32_t tail = (x0 + w) & (PPW - 1);
  auto head_pixels = (align != 0) ? static_cast<int>(PPW - align) : 0;
  auto tail_pixels = (tail != 0) ? static_cast<int>(tail) : 0;
  uint32_t mask1 = (align != 0) ? (0xffffffffU >> (head_pixels * BPP)) : 0;
  const uint32_t mask2 = 0xffffffffU << (tail_pixels * BPP);

  bool do_head = (align != 0);
  bool do_tail = (tail != 0);
  if ((align + w) < PPW) {
    mask1 |= mask2;
    do_head = true;
    do_tail = false;
    head_pixels = w;
  }
  const auto words_per_row = (w - head_pixels) >> LOG2PPW;

  auto* dst = static_cast<uint32_t*>(fb->pixels);
  dst += y0 * (fb->stride >> 2) + (x0 >> LOG2PPW);
#ifdef __MRISC32_VECTOR_OPS__
  auto rows_left = h;
  int words_left;
  uint32_t* ptr;
  uint32_t tmp;
  __asm volatile(
      "getsr   vl, #0x10\n\t"
      "mov     v1, %[color]\n"

      "1:\n\t"
      "mov     %[ptr], %[dst]\n\t"

      // Head (partial word).
      "bz      %[do_head], 2f\n\t"
      "ldw     %[tmp], %[ptr], #0\n\t"
      "add     %[ptr], %[ptr], #4\n\t"
      "sel.213 %[tmp], %[mask1], %[color]\n\t"
      "stw     %[tmp], %[ptr], #-4\n"

      "2:\n\t"
      "bz      %[words_per_row], 4f\n\t"
      "mov     %[words_left], %[words_per_row]\n"

      // Main loop (whole words).
      "3:\n\t"
      "min     vl, vl, %[words_left]\n\t"
      "sub     %[words_left], %[words_left], vl\n\t"
      "stw     v1, %[ptr], #4\n\t"
      "ldea    %[ptr], %[ptr], vl*4\n\t"
      "bgt     %[words_left], 3b\n\t"

      // Tail (partial word).
      "bz      %[do_tail], 4f\n\t"
      "ldw     %[tmp], %[ptr], #0\n\t"
      "sel.213 %[tmp], %[mask2], %[color]\n\t"
      "stw     %[tmp], %[ptr], #0\n"

      "4:\n\t"
      "add     %[rows_left], %[rows_left], #-1\n\t"
      "add     %[dst], %[dst], %[stride]\n\t"
      "bnz     %[rows_left], 1b"
      : [ dst ] "+r"(dst),
        [ ptr ] "=&r"(ptr),
        [ tmp ] "=&r"(tmp),
        [ rows_left ] "+r"(rows_left),
        [ words_left ] "=&r"(words_left)
      : [ color ] "r"(color),
        [ stride ] "r"(fb->stride),
        [ do_head ] "r"(do_head),
        [ do_tail ] "r"(do_tail),
        [ mask1 ] "r"(mask1),
        [ mask2 ] "r"(mask2),
        [ head_pixels ] "r"(head_pixels),
        [ words_per_row ] "r"(words_per_row)
      : "vl", "v1");
#else
  for (int v = 0; v < h; ++v) {
    auto* ptr = dst;

    // Head (partial word).
    if (do_head) {
      *ptr = bitmix(mask1, *ptr, color);
      ++ptr;
    }

    // Main loop (whole words).
    for (auto words_left = words_per_row; words_left > 0; --words_left) {
      *ptr++ = color;
    }

    // Tail (partial word).
    if (do_tail) {
      *ptr = bitmix(mask2, *ptr, color);
    }

    dst += (fb->stride >> 2);
  }
#endif
}

}  // namespace

extern "C" void gfx_clear(fb_t* fb, uint32_t color) {
  gfx_fill_rect(fb, 0, 0, fb->width, fb->height, color);
}

extern "C" void gfx_fill_rect(fb_t* fb, int x0, int y0, int w, int h, uint32_t color) {
  // Clamp to the framebuffer limits.
  int x1 = std::max(0, std::min(x0 + w, fb->width));
  int y1 = std::max(0, std::min(y0 + h, fb->height));
  x0 = std::max(0, x0);
  y0 = std::max(0, y0);
  w = x1 - x0;
  h = y1 - y0;
  if (w <= 0 || h <= 0) {
    return;
  }

  switch (fb->mode) {
    case CMODE_PAL1:
      gfx_fill_rect_internal<5>(fb, x0, y0, w, h, repeat32x1(color));
      break;

    case CMODE_PAL2:
      gfx_fill_rect_internal<4>(fb, x0, y0, w, h, repeat16x2(color));
      break;

    case CMODE_PAL4:
      gfx_fill_rect_internal<3>(fb, x0, y0, w, h, repeat8x4(color));
      break;

    case CMODE_PAL8:
      gfx_fill_rect_internal<2>(fb, x0, y0, w, h, repeat4x8(color));
      break;

    case CMODE_RGBA5551:
      gfx_fill_rect_internal<1>(fb, x0, y0, w, h, repeat2x16(color));
      break;

    case CMODE_RGBA8888:
      gfx_fill_rect_internal<0>(fb, x0, y0, w, h, color);
      break;
  }
}

extern "C" void gfx_draw_point(fb_t* fb, int x, int y, uint32_t color) {
  // Check if the point is inside the framebuffer limits.
  if ((x < 0) || (y < 0) || (x >= fb->width) || (y >= fb->height)) {
    return;
  }

  switch (fb->mode) {
    case CMODE_PAL1: {
      auto* ptr = &static_cast<uint8_t*>(fb->pixels)[y * fb->stride + (x >> 3)];
      const uint32_t shift = (x & 7);
      const uint32_t mask = 0x01U << shift;
      *ptr = bitmix(mask, color << shift, *ptr);
    } break;

    case CMODE_PAL2: {
      auto* ptr = &static_cast<uint8_t*>(fb->pixels)[y * fb->stride + (x >> 2)];
      const uint32_t shift = (x & 3) * 2;
      const uint32_t mask = 0x03U << shift;
      *ptr = bitmix(mask, color << shift, *ptr);
    } break;

    case CMODE_PAL4: {
      auto* ptr = &static_cast<uint8_t*>(fb->pixels)[y * fb->stride + (x >> 1)];
      const uint32_t shift = (x & 1) * 4;
      const uint32_t mask = 0x0fU << shift;
      *ptr = bitmix(mask, color << shift, *ptr);
    } break;

    case CMODE_PAL8: {
      auto* ptr = &static_cast<uint8_t*>(fb->pixels)[y * fb->stride + x];
      *ptr = static_cast<uint8_t>(color);
    } break;

    case CMODE_RGBA5551: {
      auto* ptr = &static_cast<uint16_t*>(fb->pixels)[y * (fb->stride >> 1) + x];
      *ptr = static_cast<uint16_t>(color);
    } break;

    case CMODE_RGBA8888: {
      auto* ptr = &static_cast<uint32_t*>(fb->pixels)[y * (fb->stride >> 2) + x];
      *ptr = color;
    } break;
  }
}

extern "C" void gfx_draw_line(fb_t* fb, int x0, int y0, int x1, int y1, uint32_t color) {
  // This is an implementation of Bresenham's algorithm.
  auto dx = std::abs(x1 - x0);
  auto dy = -std::abs(y1 - y0);
  int stepx = (x0 < x1) ? 1 : -1;
  int stepy = (y0 < y1) ? 1 : -1;
  auto err = dx + dy;
  auto x = x0;
  auto y = y0;
  while (true) {
    // TODO(m): Optimize me!
    gfx_draw_point(fb, x, y, color);
    if (x == x1 && y == y1)
      break;
    auto e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x += stepx;
    }
    if (e2 <= dx) {
      err += dx;
      y += stepy;
    }
  }
}
