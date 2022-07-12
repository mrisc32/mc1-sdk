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

#include <mc1/newlib_integ.h>

#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>

#include <mc1/memory.h>
#include <mc1/mfat_mc1.h>
#include <mc1/mmio.h>
#include <mc1/sdcard.h>
#include <mc1/vconsole.h>
#include <mc1/vcp.h>

//--------------------------------------------------------------------------------------------------
// Handler prototypes.
// TODO(m): These should probably be moved to a newlib-provided system header.
//--------------------------------------------------------------------------------------------------

// default-close.c
typedef int (*close_fptr_t)(int);
void _set_close_handler(close_fptr_t f);

// default-fstat.c
typedef int (*fstat_fptr_t)(int, struct stat*);
void _set_fstat_handler(fstat_fptr_t f);

// default-gettimemicros.c
typedef unsigned long long (*gettimemicros_fptr_t)(void);
void _set_gettimemicros_handler(gettimemicros_fptr_t f);

// default-lseek.c
typedef _off_t (*lseek_fptr_t)(int, _off_t, int);
void _set_lseek_handler(lseek_fptr_t f);

// default-mkdir.c
typedef int (*mkdir_fptr_t)(const char*, mode_t);
void _set_mkdir_handler(mkdir_fptr_t f);

// default-open.c
typedef int (*open_fptr_t)(const char*, int, int);
void _set_open_handler(open_fptr_t f);

// default-read.c
typedef int (*read_fptr_t)(int, char*, int);
void _set_read_handler(read_fptr_t f);

// default-rmdir.c
typedef int (*rmdir_fptr_t)(const char*);
void _set_rmdir_handler(rmdir_fptr_t f);

// default-stat.c
typedef int (*stat_fptr_t)(const char*, struct stat*);
void _set_stat_handler(stat_fptr_t f);

// default-unlink.c
typedef int (*unlink_fptr_t)(const char*);
void _set_unlink_handler(unlink_fptr_t f);

// default-write.c
typedef int (*write_fptr_t)(int, const char*, int);
void _set_write_handler(write_fptr_t f);

//--------------------------------------------------------------------------------------------------
// Internal state.
//--------------------------------------------------------------------------------------------------

static float s_microseconds_per_cycle;

static bool s_has_vcon;
static void* s_vcon_mem;

static bool s_has_sdcard;
static sdctx_t s_sdctx;
static bool s_has_fat;

//--------------------------------------------------------------------------------------------------
// Video memory allocation helper.
//--------------------------------------------------------------------------------------------------

// _end is set in the linker script, and is used by newlib malloc().
extern char _end[];

static inline bool use_malloc_for_vram(void) {
  static const char* MALLOC_HEAP_START_PTR = (char*)&_end;
  const unsigned MALLOC_HEAP_START = (const unsigned)MALLOC_HEAP_START_PTR;
  return MALLOC_HEAP_START >= VRAM_START && MALLOC_HEAP_START < XRAM_START;
}

static void* malloc_vram(size_t nbytes) {
  if (use_malloc_for_vram()) {
    return malloc(nbytes);
  }
  return mem_alloc(nbytes, MEM_TYPE_VIDEO);
}

static void free_vram(void* ptr) {
  if (use_malloc_for_vram()) {
    free(ptr);
  } else {
    mem_free(ptr);
  }
}

//--------------------------------------------------------------------------------------------------
// SDCARD I/O callbacks.
//--------------------------------------------------------------------------------------------------

static int read_block(char* ptr, unsigned block_no, void* custom) {
  sdctx_t* ctx = (sdctx_t*)(custom);
  sdcard_read(ctx, ptr, block_no, 1);
  return 0;
}

static int write_block(const char* ptr, unsigned block_no, void* custom) {
  // TODO(m): Implement me!
  (void)ptr;
  (void)block_no;
  (void)custom;
  return -1;
}

//--------------------------------------------------------------------------------------------------
// POSIX vs MFAT helpers.
//--------------------------------------------------------------------------------------------------

static int _posix_open_flags_to_mfat(int flags) {
  int mfat_flags = 0;
  if (flags & O_RDONLY)
    mfat_flags |= MFAT_O_RDONLY;
  if (flags & O_WRONLY)
    mfat_flags |= MFAT_O_WRONLY;
  if (flags & O_CREAT)
    mfat_flags |= MFAT_O_CREAT;
  return mfat_flags;
}

static mode_t _mfat_mode_to_posix(uint32_t mode) {
  mode_t posix_mode = mode & 0x1ff;  // File premissions map 1:1 to POSIX.
  if (mode & MFAT_S_IFREG)
    posix_mode |= S_IFREG;
  if (mode & MFAT_S_IFDIR)
    posix_mode |= S_IFDIR;
  return posix_mode;
}

static int _is_posix_stdout_fd(int fd) {
  return fd == STDOUT_FILENO || fd == STDERR_FILENO;
}

static int _is_posix_stdin_fd(int fd) {
  return fd == STDIN_FILENO;
}

static int _is_posix_file_fd(int fd) {
  return fd >= 3;
}

static int _posix_fd_to_mfat(int fd) {
  return fd - 3;
}

static int _mfat_fd_to_posix(int fd) {
  return fd + 3;
}

//--------------------------------------------------------------------------------------------------
// Our MC1 specific handlers.
//--------------------------------------------------------------------------------------------------

static int _mc1_close(int fd) {
  if (s_has_fat && _is_posix_file_fd(fd)) {
    return mfat_close(_posix_fd_to_mfat(fd));
  }
  errno = EBADF;
  return -1;
}

static int _mc1_fstat(int fd, struct stat* buf) {
  if (_is_posix_stdout_fd(fd)) {
    memset(buf, 0, sizeof(struct stat));
    buf->st_mode = S_IFCHR;
    buf->st_blksize = 0;
    return 0;
  }

  // TODO(m): Implement me!
  errno = EBADF;
  return -1;
}

static unsigned long long _mc1_gettimemicros(void) {
  // Safely read the two 32-bit clock counters as a single 64-bit value.
  uint32_t clk_lo, clk_hi, clk_hi_old;
  clk_hi = MMIO(CLKCNTHI);
  do {
    clk_hi_old = clk_hi;
    clk_lo = MMIO(CLKCNTLO);
    clk_hi = MMIO(CLKCNTHI);
  } while (clk_hi != clk_hi_old);
  const uint64_t clk_cnt = (((uint64_t)clk_hi) << 32) | (uint64_t)clk_lo;

  // Convert the CPU cycle count to microseconds.
  // TODO(m): We should improve the precision beyond float32.
  return (unsigned long long)(s_microseconds_per_cycle * (float)clk_cnt);
}

static _off_t _mc1_lseek(int fd, _off_t offset, int whence) {
  // TODO(m): Implement me!
  (void)fd;
  (void)offset;
  (void)whence;
  errno = EBADF;
  return (off_t)-1;
}

static int _mc1_mkdir(const char* pathname, mode_t mode) {
  // TODO(m): Implement me!
  (void)pathname;
  (void)mode;
  errno = EACCES;
  return -1;
}

static int _mc1_open(const char* pathname, int flags, int mode) {
  if (s_has_fat) {
    (void)mode;
    int fd = mfat_open(pathname, _posix_open_flags_to_mfat(flags));
    if (fd >= 0) {
      return _mfat_fd_to_posix(fd);
    }
  }
  errno = EACCES;
  return -1;
}

static int _mc1_read(int fd, char* buf, int nbytes) {
  if (_is_posix_stdin_fd(fd)) {
    // Read from the keyboard.
    // TODO(m): Implement me!
  } else if (s_has_fat && _is_posix_file_fd(fd) && (nbytes >= 0)) {
    // Read from a file.
    int64_t result = mfat_read(_posix_fd_to_mfat(fd), buf, (uint32_t)nbytes);
    if (result < 0) {
      errno = EIO;
      return -1;
    }
    return (int)result;
  }
  errno = EBADF;
  return -1;
}

static int _mc1_rmdir(const char* path) {
  // TODO(m): Implement me!
  (void)path;
  errno = EROFS;
  return -1;
}

static int _mc1_stat(const char* path, struct stat* buf) {
  if (s_has_fat) {
    mfat_stat_t stat;
    if (mfat_stat(path, &stat) == 0) {
      memset(buf, 0, sizeof(struct stat));
      buf->st_mode = _mfat_mode_to_posix(stat.st_mode);
      buf->st_size = stat.st_size;
      // TODO(m): st_mtim
      return 0;
    }
  }
  errno = ENOENT;
  return -1;
}

static int _mc1_unlink(const char* pathname) {
  // TODO(m): Implement me!
  (void)pathname;
  errno = ENOENT;
  return -1;
}

static int _mc1_write(int fd, const char* buf, int nbytes) {
  if (s_has_vcon && _is_posix_stdout_fd(fd)) {
    // Print to the text console?
    for (int i = 0; i < nbytes; ++i) {
      vcon_putc(buf[i]);
    }
    return nbytes;
  } else if (s_has_fat && _is_posix_file_fd(fd) && (nbytes >= 0)) {
    // Write to a file.
    int64_t result = mfat_write(_posix_fd_to_mfat(fd), buf, (uint32_t)nbytes);
    if (result < 0) {
      errno = EIO;
      return -1;
    }
    return (int)result;
  }
  errno = EBADF;
  return -1;
}

//--------------------------------------------------------------------------------------------------
// Public methods.
//--------------------------------------------------------------------------------------------------

void mc1newlib_init(unsigned flags) {
  // Register all handlers.
  _set_close_handler(_mc1_close);
  _set_fstat_handler(_mc1_fstat);
  _set_gettimemicros_handler(_mc1_gettimemicros);
  _set_lseek_handler(_mc1_lseek);
  _set_mkdir_handler(_mc1_mkdir);
  _set_open_handler(_mc1_open);
  _set_read_handler(_mc1_read);
  _set_rmdir_handler(_mc1_rmdir);
  _set_stat_handler(_mc1_stat);
  _set_unlink_handler(_mc1_unlink);
  _set_write_handler(_mc1_write);

  // Initialize timing functions.
  s_microseconds_per_cycle = 1000000.0F / (float)MMIO(CPUCLK);

  if (flags & MC1NEWLIB_CONSOLE) {
    // Initialize the text console.
    unsigned nbytes = vcon_memory_requirement();
    s_vcon_mem = malloc_vram(nbytes);
    if (s_vcon_mem) {
      vcon_init(s_vcon_mem);
      s_has_vcon = true;
    }
  }

  if (flags & MC1NEWLIB_SDCARD) {
    // Initialize SD-card & FAT for file I/O.
    if (sdcard_init(&s_sdctx, NULL)) {
      s_has_sdcard = true;
      if (mfat_mount(&read_block, &write_block, &s_sdctx) == 0) {
        s_has_fat = true;
      }
    }
    if (s_has_vcon && !s_has_fat) {
      vcon_print("ERROR: Unable to mount FAT partition.\n");
    }
  }
}

void mc1newlib_terminate(void) {
  if (s_has_vcon) {
    vcp_set_prg(LAYER_1, NULL);
    free_vram(s_vcon_mem);
    s_vcon_mem = NULL;
    s_has_vcon = false;
  }
  if (s_has_fat) {
    mfat_unmount();
    s_has_fat = false;
  }
  if (s_has_sdcard) {
    s_has_sdcard = false;
  }
}
