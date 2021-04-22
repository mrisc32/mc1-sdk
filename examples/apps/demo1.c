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
#include <mc1/leds.h>
#include <mc1/mci_decode.h>
#include <mc1/memory.h>
#include <mc1/mmio.h>
#include <mc1/vcp.h>

#include <stdint.h>
#include <string.h>

// Picture data (MCI encoded).
extern const unsigned char picture[];

// Font data (MCI encoded).
extern const unsigned char font_32x32[];

#define FONT_GLYPH_W 32
#define FONT_GLYPH_H 32
#define SCROLLER_W (FONT_GLYPH_W * 25)
#define SCROLLER_H FONT_GLYPH_H
#define SCROLLER_STRIDE (SCROLLER_W / 2)
#define SCROLLER_VCP_SIZE (4 * (7 + 16 + 4 * SCROLLER_H))
#define SCROLLER_Y_POS 930

// Font glyph position LUT.
// clang-format off
static const struct {
  uint8_t row;
  uint8_t col;
} FONT_POS[] = {
    // 32 - 39 [ !"#$%&']
    {2, 2}, {0, 0}, {0, 1}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {0, 5},
    // 40 - 47 [()*+,-./]
    {1, 8}, {2, 0}, {2, 2}, {2, 2}, {0, 2}, {0, 3}, {0, 4}, {2, 2},
    // 48 - 55 [01234567]
    {0, 6}, {0, 7}, {0, 8}, {0, 9}, {1, 0}, {1, 1}, {1, 2}, {1, 3},
    // 56 - 63 [89:;<=>?]
    {1, 4}, {1, 5}, {1, 6}, {1, 7}, {2, 2}, {1, 9}, {2, 2}, {2, 1},
    // 64 - 71 [@ABCDEFG]
    {2, 2}, {2, 3}, {2, 4}, {2, 5}, {2, 6}, {2, 7}, {2, 8}, {2, 9},
    // 72 - 79 [HIJKLMNO]
    {3, 0}, {3, 1}, {3, 2}, {3, 3}, {3, 4}, {3, 5}, {3, 6}, {3, 7},
    // 80 - 87 [PQRSTUVW]
    {3, 8}, {3, 9}, {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5},
    // 88 - 95 [XYZ[\]^_]
    {4, 6}, {4, 7}, {4, 8}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
};
// clang-format on

// Scroller text.
static const char SCROLL_TEXT[] =
    "                HELLO!  THIS IS A DEMONSTRATION OF THE MC1 SDK.  AWSOME ART BY "
    "FERNANDO CORREA (BE SURE TO CHECK OUT HIS OTHER WORKS) AND THE FONT IS A CLASSIC AMIGA "
    "DEMO FONT BY \"MING\" (A.K.A THE KNIGHT HAWKS FONT FROM 1988).            ";

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

static uint32_t to_abgr32(const color_t col, const uint32_t alpha) {
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
  return (alpha << 24) | (((uint32_t)ib) << 16) | (((uint32_t)ig) << 8) | (uint32_t)ir;
}

static void print_char(uint32_t* buf, int c) {
  // Start by shifting the entire scroller buffer to the left one whole glyph.
  memcpy((uint8_t*)buf,
         ((uint8_t*)buf) + (FONT_GLYPH_W / 2),
         (SCROLLER_H * SCROLLER_STRIDE) - (FONT_GLYPH_W / 2));

  // Get a pointer to the glyph in the font.
  if (c < 32 || c > 95) {
    c = 32;
  }
  c -= 32;
  int row = FONT_POS[c].row;
  int col = FONT_POS[c].col;
  const uint32_t* font_data = mci_get_raw_pixels(font_32x32);
  const uint32_t* src = &font_data[((row * 10 * FONT_GLYPH_H + col) * FONT_GLYPH_W) / 8];

  // We always print at the last (rightmost) slot in the scroller buffer.
  uint32_t* dst = &buf[(SCROLLER_W - FONT_GLYPH_W) / 8];

  // Copy the glyph.
  for (int i = 0; i < FONT_GLYPH_H; ++i) {
    for (int j = 0; j < (FONT_GLYPH_W >> 3); ++j) {
      dst[j] = src[j];
    }
    src += (FONT_GLYPH_W * 10) >> 3;
    dst += SCROLLER_W >> 3;
  }
}

int main(void) {
  // Decode the picture into a framebuffer for VCP layer 1.
  const mci_header_t* pic_hdr = mci_get_header(picture);
  if (!pic_hdr) {
    return 1;
  }
  fb_t* fb = fb_create(pic_hdr->width, pic_hdr->height, pic_hdr->pixel_format);
  if (!fb) {
    return 1;
  }
  mci_decode_pixels(picture, fb->pixels);
  mci_decode_palette(picture, fb->palette);
  fb_show(fb, LAYER_1);

  // Allocate memory for the scroller buffer.
  uint32_t* scroll_buf = mem_alloc(SCROLLER_H * SCROLLER_STRIDE, MEM_TYPE_VIDEO);

  // Set up the scroller VCP for layer 2.
  uint32_t* scroll_vcp = mem_alloc(SCROLLER_VCP_SIZE, MEM_TYPE_VIDEO);
  uint32_t* scroll_pal;
  uint32_t* scroll_xoffs;
  uint32_t* scroll_color_bar;
  {
    uint32_t* vcp = scroll_vcp;

    // VCP prologue.
    *vcp++ = vcp_emit_setreg(VCR_XINCR, (0x010000 * (SCROLLER_W - FONT_GLYPH_W)) / 1920);
    *vcp++ = vcp_emit_setreg(VCR_CMODE, CMODE_PAL4);
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0x35);

    // Palette.
    *vcp++ = vcp_emit_setpal(0, 16);
    scroll_pal = vcp;
    vcp += 16;
    mci_decode_palette(font_32x32, scroll_pal);
    scroll_pal[1] = 0x00000000;

    // Address pointers.
    uint32_t vcp_sb_addr = to_vcp_addr((uintptr_t)scroll_buf);
    uint32_t y = SCROLLER_Y_POS;
    *vcp++ = vcp_emit_waity(y);
    *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_sb_addr);
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, 1920);
    scroll_xoffs = vcp;
    *vcp++ = vcp_emit_setreg(VCR_XOFFS, 0);
    for (int k = 1; k < SCROLLER_H; ++k) {
      y += 3;
      vcp_sb_addr += SCROLLER_STRIDE / 4;
      *vcp++ = vcp_emit_waity(y);
      *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_sb_addr);
      *vcp++ = vcp_emit_setpal(2, 1);
      if (k == 1) {
        scroll_color_bar = vcp;
      }
      *vcp++ = 0x00000000;
    }
    y += 3;
    *vcp++ = vcp_emit_waity(y);
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, 0);

    // Wait forever.
    *vcp++ = vcp_emit_waity(32767);
  }
  vcp_set_prg(LAYER_2, scroll_vcp);

  // Initial scroller state.
  int scroll_text_scroll = 0;
  int scroll_text_pos = 0;

  // Set the starting theme color.
  color_t theme_col = {320.0F, 0.78F, 0.91F};  // RGB = 233, 51, 171

  // Main loop.
  while (1) {
    // Wait for the next vertical blank interval.
    wait_vbl();

    // Picture palette colors (specific to the picture that we're using):
    //  #5 = Darker
    //  #8 = Main color
    //  #12 = Less colorful
    color_t darkened = {theme_col.h, theme_col.s, theme_col.v * 0.6F};
    color_t grayened = {theme_col.h, theme_col.s * 0.5F, theme_col.v * 0.6F};
    fb->palette[5] = to_abgr32(darkened, 255);
    fb->palette[8] = to_abgr32(theme_col, 255);
    fb->palette[12] = to_abgr32(grayened, 255);

    // Scroller color bar.
    float scroll_col_h = my_mod(theme_col.h + 100.0F, 360.0F);
    for (int k = 0; k < FONT_GLYPH_H; ++k) {
      color_t scroll_col;
      scroll_col.h = scroll_col_h;
      scroll_col.s = (k & 1) ? 0.9F : 0.5F;
      scroll_col.v = (0.25F + ((1.0F - 0.25F) * (float)k) * (1.0F / FONT_GLYPH_H));
      scroll_color_bar[k * 4] = to_abgr32(scroll_col, 220);
    }

    // Update the color.
    theme_col.h += 0.5F;
    if (theme_col.h > 360.0F) {
      theme_col.h -= 360.0F;
    }

    // Update the scroll position.
    scroll_text_scroll += 3;
    if (scroll_text_scroll >= FONT_GLYPH_W) {
      scroll_text_scroll -= FONT_GLYPH_W;

      // Print a new char.
      print_char(scroll_buf, SCROLL_TEXT[scroll_text_pos++]);
      if (scroll_text_pos >= (int)sizeof(SCROLL_TEXT)) {
        scroll_text_pos = 0;
      }
    }
    *scroll_xoffs = vcp_emit_setreg(VCR_XOFFS, scroll_text_scroll << 16);

    // For profiling - print the current raster line to the 7-segment display.
    sevseg_print_dec(MMIO(VIDY));
  }
}
