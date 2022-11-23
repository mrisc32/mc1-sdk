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

#include <mc1/keyboard.h>
#include <mc1/mci_decode.h>
#include <mc1/mmio.h>
#include <mc1/vcp.h>

#include <stdint.h>

// Picture data (MCI encoded).
extern const unsigned char mc1_logo_mci[];

// Defined by the linker script.
extern char __vram_free_start;

int main(void) {
  kb_init();

  // Decode the picture.
  const mci_header_t* mci_hdr = mci_get_header(mc1_logo_mci);

  // VCP starts at the start of the free VRAM.
  uint32_t* vcp_start = (uint32_t*)&__vram_free_start;

  // Get the HW resolution.
  const uint32_t native_width = MMIO(VIDWIDTH);
  const uint32_t native_height = MMIO(VIDHEIGHT);
  const uint32_t view_height = (mci_hdr->height * native_width) / mci_hdr->width;

  // VCP prologue.
  uint32_t* vcp = vcp_start;
  *vcp++ = vcp_emit_setreg(VCR_XINCR, (0x010000U * mci_hdr->width) / native_width);
  *vcp++ = vcp_emit_setreg(VCR_CMODE, mci_hdr->pixel_format);

  // Palette.
  *vcp++ = vcp_emit_setpal(0, mci_hdr->num_pal_colors);
  mci_decode_palette(mc1_logo_mci, vcp);
  uint32_t* palette_color0 = vcp;
  vcp += mci_hdr->num_pal_colors;

  // Address pointers.
  uint32_t vcp_pixels_addr = to_vcp_addr((uintptr_t)mci_get_raw_pixels(mc1_logo_mci));
  const uint32_t vcp_pixels_stride = mci_get_stride(mci_hdr) / 4U;
  uint32_t y0 = (view_height - mci_hdr->height) / 2U;
  *vcp++ = vcp_emit_waity(y0);
  *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_pixels_addr);
  *vcp++ = vcp_emit_setreg(VCR_HSTOP, native_width);
  for (int k = 1; k < mci_hdr->height; ++k) {
    uint32_t y = y0 + (k * native_width) / mci_hdr->width;
    vcp_pixels_addr += vcp_pixels_stride;
    *vcp++ = vcp_emit_waity(y);
    *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_pixels_addr);
  }
  uint32_t y = y0 + (mci_hdr->height * native_width) / mci_hdr->width;
  *vcp++ = vcp_emit_waity(y);
  *vcp++ = vcp_emit_setreg(VCR_HSTOP, 0);

  // Wait forever.
  *vcp++ = vcp_emit_waity(32767);

  vcp_set_prg(LAYER_1, vcp_start);

  while (!kb_is_pressed(KB_ESC)) {
    // Wait for vblank.
    uint32_t old_frame_no = MMIO(VIDFRAMENO);
    uint32_t frame_no;
    do {
      frame_no = MMIO(VIDFRAMENO);
    } while (frame_no == old_frame_no);

    // Update keyboard state.
    kb_poll();

    // TODO(m): Do something more interesting here...
    *palette_color0 = 0xffe0ffe0;
  }
}
