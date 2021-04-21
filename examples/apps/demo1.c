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

#include <mc1/framebuffer.h>
#include <mc1/mci_decode.h>
#include <mc1/memory.h>
#include <mc1/mmio.h>

#include <stdint.h>

// Picture data (MCI encoded).
extern const unsigned char picture[];

typedef struct {
  float h;
  float s;
  float v;
} color_t;

static void wait_vbl(void) {
  const uint32_t old_frame_no = MMIO(VIDFRAMENO);
  while (MMIO(VIDFRAMENO) == old_frame_no)
    ;
}

static float my_abs(const float x) {
  return x < 0 ? -x : x;
}

static float my_mod(float x, const float y) {
  while (x >= y) {
    x -= y;
  }
  return x;
}

static uint32_t to_abgr32(const color_t col) {
  float c = col.s * col.v;
  float x = c * (1.0F - my_abs(my_mod((col.h * (1.0F / 60.0F)), 2.0F) - 1.0F));
  float r, g, b;
  if (col.h >= 0.0F && col.h < 60.0F) {
    r = c;
    g = x;
    b = 0.0F;
  } else if (col.h >= 60.0F && col.h < 120.0F) {
    r = x;
    g = c;
    b = 0.0F;
  } else if (col.h >= 120.0F && col.h < 180.0F) {
    r = 0.0F;
    g = c;
    b = x;
  } else if (col.h >= 180.0F && col.h < 240.0F) {
    r = 0.0F;
    g = x;
    b = c;
  } else if (col.h >= 240.0F && col.h < 300.0F) {
    r = x;
    g = 0.0F;
    b = c;
  } else {
    r = c;
    g = 0.0F;
    b = x;
  }
  float m = col.v - c;
  int ir = (int)((r + m) * 255.0F);
  int ig = (int)((g + m) * 255.0F);
  int ib = (int)((b + m) * 255.0F);
  return (255U << 24) | (((uint32_t)ib) << 16) | (((uint32_t)ig) << 8) | (uint32_t)ir;
}

int main(void) {
  // Decode the picture into a framebuffer.
  const mci_header_t* hdr = mci_get_header(picture);
  if (!hdr) {
    return 1;
  }
  fb_t* fb = fb_create(hdr->width, hdr->height, hdr->pixel_format);
  if (!fb) {
    return 1;
  }
  mci_decode_pixels(picture, fb->pixels);
  mci_decode_palette(picture, fb->palette);
  fb_show(fb, LAYER_1);

  // Change the theme color.
  color_t theme_col = {320.0F, 0.78F, 0.91F};  // RGB = 233, 51, 171
  while (1) {
    // Wait for the next vertical blank interval.
    wait_vbl();

    // Palette colors (specific to the picture that we're using):
    //  #5 = Darker
    //  #8 = Main color
    //  #12 = Less colorful
    color_t darkened = {theme_col.h, theme_col.s, theme_col.v * 0.6F};
    color_t grayened = {theme_col.h, theme_col.s * 0.5F, theme_col.v * 0.6F};
    fb->palette[5] = to_abgr32(darkened);
    fb->palette[8] = to_abgr32(theme_col);
    fb->palette[12] = to_abgr32(grayened);

    // Update the color.
    theme_col.h += 0.25F;
    if (theme_col.h > 360.0F) {
      theme_col.h -= 360.0F;
    }
  }
}
