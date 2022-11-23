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

#include <mc1/elf32.h>
#include <mc1/keyboard.h>
#include <mc1/mfat_mc1.h>
#include <mc1/mmio.h>
#include <mc1/sdcard.h>
#include <mc1/vcp.h>

#include <cstdint>
#include <cstring>

extern "C" uint8_t mc1_font_8x8[(128 - 32) * 8];
extern "C" uint8_t __vram_free_start;

namespace {

const uint32_t NUM_COLS = 120;
const uint32_t NUM_ROWS = 45;

sdctx_t s_sdctx;
uint8_t s_text[NUM_COLS * NUM_ROWS + 1];

uint8_t* align_to_4(uint8_t* ptr) {
  const auto aligned = (reinterpret_cast<uintptr_t>(ptr) + 3) & ~static_cast<uintptr_t>(3);
  return reinterpret_cast<uint8_t*>(aligned);
}

class Display {
public:
  void show_console(const uint32_t mem_start) {
    // "Allocate" memory for the text buffer, frame buffer and VCP.
    m_fb_start = reinterpret_cast<uint8_t*>(mem_start);
    m_vcp_start =
        reinterpret_cast<uint32_t*>(align_to_4(m_fb_start + GFX_HEIGHT * (GFX_WIDTH / 8U)));

    // Get the HW resolution.
    const uint32_t native_width = MMIO(VIDWIDTH);
    const uint32_t native_height = MMIO(VIDHEIGHT);

    // VCP prologue.
    uint32_t* vcp = m_vcp_start;
    *vcp++ = vcp_emit_setreg(VCR_XINCR, (0x010000U * GFX_WIDTH) / native_width);
    *vcp++ = vcp_emit_setreg(VCR_CMODE, CMODE_PAL1);

    // Palette.
    *vcp++ = vcp_emit_setpal(0, 2);
    *vcp++ = PALETTE_COLOR_0;
    *vcp++ = PALETTE_COLOR_1;

    // Address pointers.
    uint32_t vcp_pixels_addr = to_vcp_addr(reinterpret_cast<uintptr_t>(m_fb_start));
    const uint32_t vcp_pixels_stride = GFX_WIDTH / (8U * 4U);
    uint32_t y0 = 0U;
    *vcp++ = vcp_emit_waity(y0);
    *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_pixels_addr);
    *vcp++ = vcp_emit_setreg(VCR_HSTOP, native_width);
    for (uint32_t k = 1U; k < GFX_HEIGHT; ++k) {
      uint32_t y = y0 + (k * native_height) / GFX_HEIGHT;
      vcp_pixels_addr += vcp_pixels_stride;
      *vcp++ = vcp_emit_waity(y);
      *vcp++ = vcp_emit_setreg(VCR_ADDR, vcp_pixels_addr);
    }

    // Wait forever.
    *vcp++ = vcp_emit_waity(32767);

    vcp_set_prg(LAYER_1, m_vcp_start);
    vcp_set_prg(LAYER_2, nullptr);
  }

  void show_load_screen() {
    // TODO(m): Show something more fancy?
    vcp_set_prg(LAYER_1, nullptr);
  }

  void wait_vbl() {
    // Wait for vblank.
    const auto old_frame_no = MMIO(VIDFRAMENO);
    uint32_t frame_no;
    do {
      frame_no = MMIO(VIDFRAMENO);
    } while (frame_no == old_frame_no);

    // Update counters.
    m_blink_count = (m_blink_count + frame_no - m_frame_no) % (BLINK_INTERVAL * 2U);
    m_frame_no = frame_no;
  }

  void refresh() {
    paint_text();
  }

  void clear() {
    memset(&s_text[0], ' ', NUM_COLS * NUM_ROWS);
    m_x = 0;
    m_y = 0;
  }

  void putc(const int c) {
    if (c == '\n') {
      m_x = 0;
      move_y(1);
    } else if (c >= 32 && c <= 127) {
      s_text[(m_y * NUM_COLS) + m_x] = static_cast<uint8_t>(c);
      move_x(1);
    }
  }

  void puts(const char* s) {
    while (*s != 0) {
      putc(*s);
      s++;
    }
  }

  void puthex(const uint32_t x) {
    static const char HEX[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    for (int s = 28; s >= 0; s -= 4) {
      putc(HEX[(x >> s) & 0xfU]);
    }
  }

  void del() {
    auto* row = &s_text[NUM_COLS * m_y];
    if (m_x < (NUM_COLS - 1)) {
      memmove(row + m_x, row + m_x + 1, NUM_COLS - m_x - 1);
    }
    row[NUM_COLS - 1] = ' ';
  }

  void backspace() {
    if (m_x > 0U) {
      --m_x;
      del();
    }
  }

  void insert() {
    auto* row = &s_text[NUM_COLS * m_y];
    if (m_x < (NUM_COLS - 1)) {
      memmove(row + m_x + 1, row + m_x, NUM_COLS - m_x - 1);
    }
    row[m_x] = ' ';
  }

  void move_x(const int dir) {
    const auto new_x = static_cast<uint32_t>(static_cast<int>(m_x) + dir + NUM_COLS) % NUM_COLS;
    if (dir > 0 && new_x < m_x) {
      move_y(1);
    } else if (dir < 0 && new_x > m_x) {
      move_y(-1);
    }
    m_x = new_x;
  }

  void move_y(const int dir) {
    auto new_y = static_cast<int>(m_y) + dir;
    if (new_y >= static_cast<int>(NUM_ROWS)) {
      // Scroll up entire screen.
      memmove(&s_text[0], &s_text[NUM_COLS], NUM_COLS * (NUM_ROWS - 1));
      memset(&s_text[NUM_COLS * (NUM_ROWS - 1)], ' ', NUM_COLS);
      new_y = NUM_ROWS - 1;
    } else if (new_y < 0) {
      new_y = 0;
    }
    m_y = static_cast<uint32_t>(new_y);
  }

  uint32_t frame_no() const {
    return m_frame_no;
  }

  char* current_line() {
    return reinterpret_cast<char*>(&s_text[m_y * NUM_COLS]);
  }

private:
  static const uint32_t BLINK_INTERVAL = 32;
  static const uint32_t GFX_WIDTH = NUM_COLS * 8;
  static const uint32_t GFX_HEIGHT = NUM_ROWS * 8;
  static const uint32_t PALETTE_COLOR_0 = 0xff305030U;
  static const uint32_t PALETTE_COLOR_1 = 0xff80ff80U;

  void paint_text() const {
    const auto* text = &s_text[0];
    auto* dst = m_fb_start;
    const int blink_state = (m_blink_count < BLINK_INTERVAL) ? -1 : 0;
    for (uint32_t y = 0U; y < NUM_ROWS; ++y) {
      const int on_cursor_y = (y == m_y) ? -1 : 0;
      for (uint32_t x = 0U; x < NUM_COLS; ++x) {
        auto c = static_cast<uint32_t>(*text++);
        const auto* glyph = &mc1_font_8x8[(c - 32) * 8];

        const int on_cursor_x = (x == m_x) ? -1 : 0;
        const int invert = blink_state & on_cursor_x & on_cursor_y;

#if defined(__MRISC32_VECTOR_OPS__)
        static_assert(NUM_COLS == 120);
        // clang-format off
        __asm__ volatile(
            "ldi     vl, #8\n\t"
            "ldub    v1, [%[glyph], #1]\n\t"
            "xor     v1, v1, %[invert]\n\t"
            "stb     v1, [%[dst], #%[NUM_COLS]]"
            :
            : [glyph] "r"(glyph),
              [dst] "r"(dst),
              [invert] "r"(invert),
              [NUM_COLS] "i"(NUM_COLS)
            : "vl", "v1"
        );
        // clang-format on
#else
        dst[NUM_COLS * 0] = glyph[0] ^ invert;
        dst[NUM_COLS * 1] = glyph[1] ^ invert;
        dst[NUM_COLS * 2] = glyph[2] ^ invert;
        dst[NUM_COLS * 3] = glyph[3] ^ invert;
        dst[NUM_COLS * 4] = glyph[4] ^ invert;
        dst[NUM_COLS * 5] = glyph[5] ^ invert;
        dst[NUM_COLS * 6] = glyph[6] ^ invert;
        dst[NUM_COLS * 7] = glyph[7] ^ invert;
#endif
        ++dst;
      }
      dst += NUM_COLS * 7;
    }
  }

  uint8_t* m_fb_start;
  uint32_t* m_vcp_start;

  uint32_t m_frame_no = 0;
  uint32_t m_blink_count = 0;

  uint32_t m_x;
  uint32_t m_y;
};

class Shell {
public:
  void run() {
    m_display.show_console(VRAM_FREE_START);
    m_display.clear();
    m_display.puts("Welcome to the MRISC32 Shell!\n\n");

    m_display.puts("VRAM: 0x40000000-0x");
    m_display.puthex(0x40000000U + MMIO(VRAMSIZE) - 1);
    const auto xram_size = MMIO(XRAMSIZE);
    if (xram_size != 0U) {
      m_display.puts("\nXRAM: 0x80000000-0x");
      m_display.puthex(0x80000000U + MMIO(XRAMSIZE) - 1);
    }
    m_display.puts("\nFree VRAM start: 0x");
    m_display.puthex(VRAM_FREE_START);
    m_display.putc('\n');

    if (mount_filesystem()) {
      m_display.puts("Successfully mounted filesystem.\n");
    }
    kb_init();

    m_display.putc('\n');
    new_prompt();
    while (!m_terminate) {
      m_display.wait_vbl();
      m_display.refresh();

      handle_keyboard_input();
      handle_buttons();
    }
  }

private:
  const uint32_t VRAM_FREE_START = reinterpret_cast<uint32_t>(&__vram_free_start);

  void handle_keyboard_input() {
    kb_poll();
    uint32_t event;
    while ((event = kb_get_next_event()) != 0U) {
      if (kb_event_is_press(event)) {
        switch (kb_event_scancode(event)) {
          case KB_ENTER:
            handle_enter();
            break;
          case KB_LEFT:
            m_display.move_x(-1);
            break;
          case KB_RIGHT:
            m_display.move_x(1);
            break;
          case KB_UP:
            m_display.move_y(-1);
            break;
          case KB_DOWN:
            m_display.move_y(1);
            break;
          case KB_BACKSPACE:
            m_display.backspace();
            break;
          case KB_DEL:
            m_display.del();
            break;
          case KB_INSERT:
            m_display.insert();
            break;
          default:
            m_display.putc(kb_event_to_char(event));
        }
      }
    }
  }

  void handle_buttons() {
    // Detect newly pushed buttons.
    const auto btns = MMIO(BUTTONS);
    const auto pushed_btns = btns & ~m_old_btns;
    m_old_btns = btns;

    if (pushed_btns & 0x00000001U) {
      m_display.move_y(-1);
    } else if (pushed_btns & 0x00000002U) {
      m_display.move_y(1);
    } else if (pushed_btns & 0x00000004U) {
      handle_enter();
    } else if (pushed_btns & 0x00000008U) {
      m_display.puts("ls");
      handle_enter();
    }
  }

  void handle_enter() {
    auto* cmd = m_display.current_line();
    m_display.putc('\n');
    handle_command(cmd);
    new_prompt();
  }

  void handle_command(char* cmd) {
    // Extract the command part.
    // Note: We teporarily zero-terminate the string in the screen buffer in order to avoid having
    // to allocate/duplicate the string. It's OK to add a zero after the last row in the text buffer
    // since we have allocated one byte more than we need.
    uint32_t len;
    for (len = 0U; (len < NUM_COLS) && (cmd[len] != ' '); ++len) {
    }
    const auto old_char = cmd[len];
    cmd[len] = 0;

    // Handle built-in commands.
    if (len == 2 && cmd[0] == 'l' && cmd[1] == 's') {
      execute_ls();
    } else if (len == 4 && cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'i' && cmd[3] == 't') {
      execute_exit();
    } else if (len > 0U) {
      execute_program(cmd);
    }

    // Restore the char in the text buffer (drop zero termination).
    cmd[len] = old_char;
  }

  void execute_ls() {
    m_display.puts("ls - not yet implemented\n");
  }

  void execute_exit() {
    m_terminate = true;
  }

  void execute_program(const char* exe_name) {
    // Hide the console.
    m_display.show_load_screen();
    m_display.wait_vbl();

    // Try to load the program.
    uint32_t entry_address = 0U;
    bool success = elf32_load(exe_name, &entry_address);

    if (success) {
      // Start the loaded application.
      using entry_fun_t = int(int, const char**);
      auto* entry_fun = reinterpret_cast<entry_fun_t*>(entry_address);
      const char* argv[1] = {exe_name};
      (void)entry_fun(1, argv);

      // Restore the console.
      m_display.show_console(VRAM_FREE_START);
    } else {
      // Restore the console and print an error message.
      m_display.show_console(VRAM_FREE_START);
      m_display.puts("*** Failed to load program: <");
      m_display.puts(exe_name);
      m_display.puts(">\n");
    }
  }

  void new_prompt() {
    m_display.puts("[MRSH] Ready>\n");
  }

  bool mount_filesystem(bool force = false) {
    if (force || !m_filesystem_mounted) {
      if (sdcard_init(&s_sdctx, nullptr)) {
        const auto result = mfat_mount(read_block_fun, write_block_fun, nullptr);
        m_filesystem_mounted = (result == 0);
      }
    }
    return m_filesystem_mounted;
  }

  static int read_block_fun(char* ptr, unsigned block_no, void*) {
    return sdcard_read(&s_sdctx, ptr, block_no, 1) ? 0 : -1;
  }

  static int write_block_fun(const char*, unsigned, void*) {
    // Not implemented.
    return -1;
  }

  bool m_terminate = false;
  bool m_filesystem_mounted = false;
  Display m_display;
  uint32_t m_old_btns = 0U;
};

}  // namespace

int main(void) {
  Shell shell;
  shell.run();
}
