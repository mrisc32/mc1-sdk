// -*- mode: c; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
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

#ifndef MC1_MMIO_H_
#define MC1_MMIO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// MMIO registers.
typedef volatile struct {
  uint32_t CLKCNTLO;
  uint32_t CLKCNTHI;
  uint32_t CPUCLK;
  uint32_t VRAMSIZE;
  uint32_t XRAMSIZE;
  uint32_t VIDWIDTH;
  uint32_t VIDHEIGHT;
  uint32_t VIDFPS;
  uint32_t VIDFRAMENO;
  uint32_t VIDY;
  uint32_t SWITCHES;
  uint32_t BUTTONS;
  uint32_t KEYPTR;
  uint32_t MOUSEPOS;
  uint32_t MOUSEBTNS;
  uint32_t SDIN;
  uint32_t SEGDISP0;
  uint32_t SEGDISP1;
  uint32_t SEGDISP2;
  uint32_t SEGDISP3;
  uint32_t SEGDISP4;
  uint32_t SEGDISP5;
  uint32_t SEGDISP6;
  uint32_t SEGDISP7;
  uint32_t LEDS;
  uint32_t SDOUT;
  uint32_t SDWE;
} mmio_regs_t;

// Macro for accessing MMIO registers.
#ifdef __cplusplus
#define MMIO(reg) (reinterpret_cast<mmio_regs_t*>(0xc0000000)->reg)
#else
#define MMIO(reg) (((mmio_regs_t*)0xc0000000)->reg)
#endif

// Macro for reading the key event buffer.
// The key event buffer is a 16-entry circular buffer (each entry is a 32-bit word), starting at
// 0xc00080.
#ifdef __cplusplus
#define KEYBUF(ptr) \
  (reinterpret_cast<volatile uint32_t*>(reinterpret_cast<volatile uint8_t*>(0xc0000080)))[ptr]
#else
#define KEYBUF(ptr) ((volatile uint32_t*)(((volatile uint8_t*)0xc0000080)))[ptr]
#endif

// Number of entires in the key event buffer.
#define KEYBUF_SIZE 16

// SPI SD card I/O bits (SDIN, SDOUT, SDWE).
#define SD_MISO_BIT_NO 0
#define SD_MISO_BIT    (1 << SD_MISO_BIT_NO)
#define SD_CS_BIT_NO   3
#define SD_CS_BIT      (1 << SD_CS_BIT_NO)
#define SD_MOSI_BIT_NO 4
#define SD_MOSI_BIT    (1 << SD_MOSI_BIT_NO)
#define SD_SCK_BIT_NO  5
#define SD_SCK_BIT     (1 << SD_SCK_BIT_NO)

// SD mode SD card I/O bits (SDIN, SDOUT, SDWE).
#define SD_DAT0_BIT_NO 0
#define SD_DAT0_BIT    (1 << SD_DAT0_BIT_NO)
#define SD_DAT1_BIT_NO 1
#define SD_DAT1_BIT    (1 << SD_DAT1_BIT_NO)
#define SD_DAT2_BIT_NO 2
#define SD_DAT2_BIT    (1 << SD_DAT2_BIT_NO)
#define SD_DAT3_BIT_NO 3
#define SD_DAT3_BIT    (1 << SD_DAT3_BIT_NO)
#define SD_CMD_BIT_NO  4
#define SD_CMD_BIT     (1 << SD_CMD_BIT_NO)
#define SD_CLK_BIT_NO  5
#define SD_CLK_BIT     (1 << SD_CLK_BIT_NO)

#ifdef __cplusplus
}
#endif

#endif  // MC1_MMIO_H_

