/* -*- mode: ld-script; tab-width: 4; indent-tabs-mode: nil; -*-             */
/* ------------------------------------------------------------------------- */
/* Copyright (c) 2021 Marcus Geelnard                                        */
/*                                                                           */
/* This software is provided 'as-is', without any express or implied         */
/* warranty. In no event will the authors be held liable for any damages     */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/*  1. The origin of this software must not be misrepresented; you must not  */
/*     claim that you wrote the original software. If you use this software  */
/*     in a product, an acknowledgment in the product documentation would be */
/*     appreciated but is not required.                                      */
/*                                                                           */
/*  2. Altered source versions must be plainly marked as such, and must not  */
/*     be misrepresented as being the original software.                     */
/*                                                                           */
/*  3. This notice may not be removed or altered from any source             */
/*     distribution.                                                         */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* Linker script for MC1 applications that load into VRAM                    */
/* ------------------------------------------------------------------------- */

OUTPUT_FORMAT("elf32-mrisc32")
OUTPUT_ARCH("mrisc32")
ENTRY(_start)

__vram_start = 0x40001000;   /* Start of VRAM (w room for ROM BSS) */
__xram_start = 0x80000000;   /* Start of XRAM. */
__app_start  = __vram_start; /* Start address for the application. */

SECTIONS
{
    /* --------------------------------------------------------------------- */
    /* Code.                                                                 */
    /* --------------------------------------------------------------------- */

    . = __app_start;

    .text :
    {
        *(.text.start)  /* Ensure that the _start() function is placed first */
        *(.text*)
        KEEP (*(SORT_NONE(.init)))
        KEEP (*(SORT_NONE(.fini)))
    }

    . = ALIGN(32);
    .ctor :
    {
        __CTOR_START = .;
        KEEP (*crtbegin.o(.ctors))
        KEEP (*crtbegin?.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
        __CTOR_END = .;
    }

    . = ALIGN(32);
    .dtor :
    {
        __DTOR_START = .;
        KEEP (*crtbegin.o(.dtors))
        KEEP (*crtbegin?.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
        __DTOR_END = .;
    }

    . = ALIGN(32);
    .eh_frame :
    {
        *(.eh_frame*)
    }


    /* --------------------------------------------------------------------- */
    /* Data.                                                                 */
    /* --------------------------------------------------------------------- */

    . = ALIGN(32);
    .rodata :
    {
        *(.rodata*)
    }

    . = ALIGN(32);
    .data :
    {
        *(.data*)
    }

    . = ALIGN(32);
    .sdata :
    {
        *(.sdata*)
    }

    __app_size = . - __app_start;


    /* --------------------------------------------------------------------- */
    /* BSS goes after the application code and data.                         */
    /* We define __bss_start and __bss_size so the startup code knows what   */
    /* memory area to clear.                                                 */
    /* --------------------------------------------------------------------- */

    . = ALIGN(32);
    __bss_start = .;

    .sbss (NOLOAD) :
    {
        *(.sbss*)
        *(.scommon*)
    }

    . = ALIGN(32);
    .bss (NOLOAD) :
    {
        *(.bss*)
        *(COMMON)
    }

    . = ALIGN(4);
    __bss_size = . - __bss_start;


    /* This tells the system where it can start to allocate VRAM. */
    . = ALIGN(32);
    __vram_free_start = .;

    /* This tells the system where it can start to allocate XRAM. */
    __xram_free_start = __xram_start;
}
