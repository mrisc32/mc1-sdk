// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
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

// External symbols.
extern "C" uint8_t mc1_font_8x8[(128 - 32) * 8];
extern "C" uint8_t __vram_free_start;

namespace {

// Screen dimensions.
constexpr uint32_t NUM_COLS = 120U;
constexpr uint32_t NUM_ROWS = 45U;

// Limits.
constexpr uint32_t MAX_PATH_LEN = 127U;
constexpr int MAX_NUM_ARGS = 8U;

// Static variables / state.
sdctx_t s_sdctx;
uint8_t s_text[NUM_COLS * NUM_ROWS];
char s_cmdline[NUM_COLS + 1];
char s_cwd[MAX_PATH_LEN + 1];
char s_abs_path[MAX_PATH_LEN + 1];
char s_tmp_path[MAX_PATH_LEN + 1];

uint8_t* align_to_4(uint8_t* ptr) {
  const auto aligned = (reinterpret_cast<uintptr_t>(ptr) + 3) & ~static_cast<uintptr_t>(3);
  return reinterpret_cast<uint8_t*>(aligned);
}

void append_path(char* result, const char* base, const char* path) {
  // TODO(m): Handle ".", ".." and superfluous "/".
  uint32_t len = 0U;
  for (; base[len] != 0; ++len) {
    result[len] = base[len];
  }

  if (len > 0U && base[len - 1] != '/' && len < MAX_PATH_LEN) {
    result[len++] = '/';
  }

  for (; *path != 0 && len < MAX_PATH_LEN; ++len) {
    result[len] = *path++;
  }

  result[len] = 0;
}

const char* make_full_path(const char* path) {
  // Absolute path?
  if (path[0] == '/') {
    return path;
  }

  // Turn the relative path into an absolute path.
  append_path(s_abs_path, s_cwd, path);

  return s_abs_path;
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
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0U);  // Turn of dithering.
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
    // Set a constant backround color.
    auto* vcp = reinterpret_cast<uint32_t*>(0x40000010U);  // Layer 1 VCP start.
    *vcp++ = vcp_emit_setreg(VCR_RMODE, 0U);               // Turn of dithering.
    *vcp++ = vcp_emit_setpal(0, 1);
    *vcp++ = 0xff88aa55U;
    *vcp++ = vcp_emit_waity(32767);
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

  void putdec(uint32_t x, int min_width = 0, bool expand = false) {
    char buf[11];
    buf[10] = 0;
    int i = 10;
    int width = 1;
    while (i > 0) {
      buf[--i] = '0' + (x % 10U);
      x /= 10U;
      if (x == 0U && width >= min_width) {
        break;
      }
      ++width;
    }
    if (expand) {
      while (i > 0) {
        buf[--i] = ' ';
      }
    }
    puts(&buf[i]);
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
    // Set the default working dir.
    s_cwd[0] = '/';
    s_cwd[1] = 0;

    // Display the welcome text.
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

    // Initialize I/O.
    if (mount_filesystem()) {
      m_display.puts("Successfully mounted filesystem.\n");
    }
    kb_init();

    // Show the prompt.
    m_display.putc('\n');
    new_prompt();

    // Main loop.
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
    memcpy(s_cmdline, m_display.current_line(), NUM_COLS);
    s_cmdline[NUM_COLS] = 0;

    m_display.putc('\n');
    handle_command(s_cmdline);
    new_prompt();
  }

  void handle_command(char* cmdline) {
    // Extract the command parts and zero terminate each part.
    const char* argv[MAX_NUM_ARGS];
    int argc = 0;
    char* arg = cmdline;
    while (*arg != 0 && argc < MAX_NUM_ARGS) {
      // Find start of argument.
      for (; *arg != 0 && *arg == ' '; ++arg) {
      }
      char end_char = (*arg == '"') ? '"' : ' ';
      if (end_char != ' ') {
        ++arg;
      }
      argv[argc] = arg;

      // Find end of argument.
      uint32_t len = 0U;
      for (; *arg != 0 && *arg != end_char; ++arg, ++len) {
      }
      if (len > 0U || (end_char != ' ' && *arg == end_char)) {
        ++argc;
      }
      if (*arg != 0) {
        // Zero-terminate the argument.
        *arg++ = 0;
      }
    }

    // Nothing to do?
    if (argc == 0) {
      return;
    }

    // Handle built-in commands.
    const auto* cmd = argv[0];
    if (cmd[0] == 'l' && cmd[1] == 's' && cmd[2] == 0) {
      execute_ls(argc, argv);
    } else if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == 0) {
      execute_cd(argc, argv);
    } else if (cmd[0] == 'p' && cmd[1] == 'w' && cmd[2] == 'd' && cmd[3] == 0) {
      execute_pwd(argc, argv);
    } else if (cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'i' && cmd[3] == 't' && cmd[4] == 0) {
      execute_exit(argc, argv);
    } else {
      execute_program(argc, argv);
    }
  }

  void execute_ls(int argc, const char** argv) {
    // Parse arguments.
    const auto* path = s_cwd;
    auto flag_l = false;
    auto flag_F = false;
    for (int k = 1; k < argc; ++k) {
      if (argv[k][0] == '-') {
        const auto* flag = &argv[k][1];
        for (; *flag != 0; ++flag) {
          if (*flag == 'l') {
            flag_l = true;
          } else if (*flag == 'F') {
            flag_F = true;
          }
        }
      } else {
        path = make_full_path(argv[k]);
      }
    }

    // Print directory contents.
    auto* dirp = mfat_opendir(path);
    if (dirp != nullptr) {
      mfat_dirent_t* dirent;
      while ((dirent = mfat_readdir(dirp)) != nullptr) {
        // Stat the dirent to get more info.
        mfat_stat_t s;
        append_path(s_tmp_path, path, dirent->d_name);
        if (mfat_stat(s_tmp_path, &s) != 0) {
          memset(&s, 0, sizeof(s));
        }

        // Print information about the directory entry.
        if (flag_l) {
          // Mode flags.
          auto mode = s.st_mode;
          m_display.putc(MFAT_S_ISDIR(mode) ? 'd' : '-');
          for (int i = 0; i < 3; ++i) {
            m_display.putc((mode & MFAT_S_IRUSR) != 0U ? 'r' : '-');
            m_display.putc((mode & MFAT_S_IWUSR) != 0U ? 'w' : '-');
            m_display.putc((mode & MFAT_S_IXUSR) != 0U ? 'x' : '-');
            mode <<= 3;
          }

          // File size.
          m_display.putdec(s.st_size, 0, true);
          m_display.putc(' ');

          // Modification time.
          m_display.putdec(s.st_mtim.year, 4);
          m_display.putc('-');
          m_display.putdec(s.st_mtim.month, 2);
          m_display.putc('-');
          m_display.putdec(s.st_mtim.day, 2);
          m_display.putc(' ');
          m_display.putdec(s.st_mtim.hour, 2);
          m_display.putc(':');
          m_display.putdec(s.st_mtim.minute, 2);
          m_display.putc(' ');
        }

        // File/dir name.
        m_display.puts(dirent->d_name);

        // Add suitable suffix when -F is used.
        if (flag_F) {
          if (MFAT_S_ISDIR(s.st_mode)) {
            m_display.putc('/');
          } else if ((s.st_mode & MFAT_S_IXUSR) != 0U) {
            m_display.putc('*');
          }
        }

        m_display.putc('\n');
      }
      mfat_closedir(dirp);
    }
  }

  void execute_pwd(int argc, const char** argv) {
    // Ignore arguments.
    (void)argc;
    (void)argv;

    m_display.puts(s_cwd);
    m_display.putc('\n');
    return;
  }

  void execute_cd(int argc, const char** argv) {
    // Default to the root dir.
    const char* new_cwd = "/";
    if (argc > 1) {
      new_cwd = make_full_path(argv[1]);
    }

    int len;
    for (len = 0; *new_cwd != 0; ++len) {
      s_cwd[len] = *new_cwd++;
    }
    s_cwd[len] = 0;
  }

  void execute_exit(int argc, const char** argv) {
    // Ignore arguments.
    (void)argc;
    (void)argv;

    m_terminate = true;
  }

  void execute_program(int argc, const char** argv) {
    // Hide the console.
    m_display.show_load_screen();
    m_display.wait_vbl();

    // Try to load the program.
    // TODO(m): Take CWD into account.
    uint32_t entry_address = 0U;
    bool success = elf32_load(argv[0], &entry_address);

    if (success) {
      // Start the loaded application.
      using entry_fun_t = int(int, const char**);
      auto* entry_fun = reinterpret_cast<entry_fun_t*>(entry_address);
      (void)entry_fun(argc, argv);

      // Restore the console.
      m_display.show_console(VRAM_FREE_START);

      // Get back to a known good state.
      mount_filesystem(true);
      kb_init();
    } else {
      // Restore the console and print an error message.
      m_display.show_console(VRAM_FREE_START);
      m_display.puts("*** Failed to load program: <");
      m_display.puts(argv[0]);
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
