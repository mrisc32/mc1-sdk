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

inline void gfx_fill_rect_kernel(uint32_t* dst, int w, int h, int stride, uint32_t color) {
#ifdef __MRISC32_VECTOR_OPS__
  auto rows_left = h;
  int cols_left;
  uint32_t* ptr;
  __asm volatile(
      "cpuid   vl,z,z\n\t"
      "mov     v1,%[color]\n"
      "1:\n\t"
      "mov     %[ptr],%[dst]\n\t"
      "mov     %[cols_left],%[w]\n"
      "2:\n\t"
      "minu    vl,%[cols_left],vl\n\t"
      "sub     %[cols_left],%[cols_left],vl\n\t"
      "stw     v1,%[ptr],#4\n\t"
      "ldea    %[ptr],%[ptr],vl*4\n\t"
      "bnz     %[cols_left],2b\n"
      "\n\t"
      "add     %[rows_left],%[rows_left],#-1\n\t"
      "add     %[dst],%[dst],%[stride]\n\t"
      "bnz     %[rows_left],1b"
      : [ dst ] "+r"(dst),
        [ ptr ] "=&r"(ptr),
        [ rows_left ] "+r"(rows_left),
        [ cols_left ] "=&r"(cols_left)
      : [ color ] "r"(color), [ w ] "r"(w), [ stride ] "r"(stride)
      : "vl", "v1");
#else
  for (int v = 0; v < h; ++v) {
    for (int u = 0; u < w; ++u) {
      dst[u] = color;
    }
    dst += stride >> 2;
  }
#endif
}

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
  uint32_t mask1 = (PPW > 1 && align != 0) ? (0xffffffffU >> ((PPW - align) * BPP)) : 0;
  const uint32_t mask2 = tail ? (0xffffffffU << (tail * BPP)) : 0;

  bool do_align = (align != 0);
  if ((align + w) < PPW) {
    mask1 |= mask2;
    do_align = true;
  }

  auto* row = static_cast<uint32_t*>(fb->pixels);
  row += y0 * (fb->stride >> 2) + (x0 >> LOG2PPW);
  for (int v = 0; v < h; ++v) {
    auto* ptr = row;
    int pixels_left = w;

    // Head alignment (partial word).
    if (do_align) {
      *ptr = bitmix(mask1, *ptr, color);
      ++ptr;
      pixels_left -= std::min(w, static_cast<int>(PPW - align));
    }

    // Main loop (whole words).
    // TODO(m): Vectorize this loop.
    auto words_left = pixels_left >> LOG2PPW;
    pixels_left -= words_left << LOG2PPW;
    for (; words_left > 0; --words_left) {
      *ptr++ = color;
    }

    // Tail (partial word).
    if (pixels_left > 0) {
      *ptr = bitmix(mask2, *ptr, color);
    }

    row += (fb->stride >> 2);
  }
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

    case CMODE_RGBA8888: {
      auto* row = static_cast<uint32_t*>(fb->pixels);
      row += y0 * (fb->stride >> 2) + x0;
      gfx_fill_rect_kernel(row, w, h, fb->stride, color);
    } break;
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
