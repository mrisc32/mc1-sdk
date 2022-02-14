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
#include <mc1/mfat_mc1.h>
#include <mc1/sdcard.h>
#include <mc1/vconsole.h>

#include <stdlib.h>
#include <string.h>

// Defined by the linker script.
extern char __vram_free_start;

#define MAX_PATH_LENGTH 32
#define MAX_PROGRAMS 16

static const char* PROGRAMS_LST_FILE = "PROGRAMS.LST";
static char s_programs[MAX_PROGRAMS][MAX_PATH_LENGTH];

static int read_block_fun(char* ptr, unsigned block_no, void* custom) {
  sdctx_t* ctx = (sdctx_t*)custom;
  return sdcard_read(ctx, ptr, block_no, 1) ? 0 : -1;
}

int write_block_fun(const char*, unsigned, void*) {
  // DUMMY
}

void sdcard_log_func(const char* msg) {
  vcon_print(msg);
}

int main(void) {
  // Initialize VCON.
  void* vcon_start = (void*)&__vram_free_start;
  vcon_init(vcon_start);
  vcon_show(LAYER_1);
  vcon_print("Loading program list...\n");

  // Mount the SD card.
  sdctx_t sdctx;
  if (!sdcard_init(&sdctx, sdcard_log_func)) {
    vcon_print("*** Failed to initialize SD card\n");
    exit(1);
  }
  if (mfat_mount(read_block_fun, write_block_fun, &sdctx) != 0) {
    vcon_print("*** Failed to mount SD card\n");
    exit(1);
  }

  // Read the program list.
  int lst_fd = mfat_open(PROGRAMS_LST_FILE, MFAT_O_RDONLY);
  if (lst_fd == -1) {
    vcon_print("*** Failed to open ");
    vcon_print(PROGRAMS_LST_FILE);
    vcon_print("\n");
    exit(1);
  }
  for (int prg_no = 0; prg_no < MAX_PROGRAMS; ++prg_no) {
    char buf[1];
    int eol_reached = 0;
    int pos = 0;
    memset(&s_programs[prg_no][0], 0, MAX_PATH_LENGTH);
    while (!eol_reached) {
      int64_t bytes_read = mfat_read(lst_fd, &buf[0], 1);
      if (bytes_read == 1) {
        unsigned c = (unsigned)buf[0];
        if (c == 0x0a || c == 0x0d) {
          eol_reached = 1;
        } else if (pos < MAX_PATH_LENGTH - 1) {
          s_programs[prg_no][pos++] = c;
        }
      } else {
        eol_reached = 1;
      }
    }
  }
  mfat_close(lst_fd);

  // Initialize the keyboard.
  kb_init();

  int selected_prg = 0;
  while (1) {
    // Print the list.
    vcon_clear();
    for (int prg_no = 0; prg_no < MAX_PROGRAMS; ++prg_no) {
      const char* prefix = "   ";
      if (prg_no == selected_prg) {
        prefix = "-> ";
      }
      vcon_print(prefix);
      vcon_print(&s_programs[prg_no][0]);
      vcon_print("\n");
    }

    // Wait for input.
    int select_delta = 0;
    while (select_delta == 0) {
      // Wait for the next keyboard press event.
      uint32_t event = 0U;
      while (event == 0U) {
        kb_poll();
        event = kb_get_next_event();
        if (!kb_event_is_press(event)) {
          event = 0U;
        }
      }
      uint32_t scancode = kb_event_scancode(event);

      if (scancode == KB_UP) {
        select_delta = -1;
      } else if (scancode == KB_DOWN) {
        select_delta = 1;
      } else if (scancode == KB_ENTER || KB_ENTER == KB_SPACE) {
        // TODO(m): Load the program using an ELF32 loader!
        vcon_print("\nLOADING: ");
        vcon_print(&s_programs[selected_prg][0]);
        vcon_print(" ...\n");
      }
    }

    // Update the selected program number.
    int new_selected_prg = selected_prg + select_delta;
    if (new_selected_prg < 0) {
      new_selected_prg = 0;
    } else if (new_selected_prg >= MAX_PROGRAMS) {
      new_selected_prg = MAX_PROGRAMS - 1;
    }
  }
}
