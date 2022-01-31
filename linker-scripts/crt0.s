; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This file contains the common startup code. It defines _start, which does
; some initialization and then calls main.
; ----------------------------------------------------------------------------
; Copyright (c) 2021 Marcus Geelnard
;
; This software is provided 'as-is', without any express or implied warranty.
; In no event will the authors be held liable for any damages arising from
; the use of this software.
;
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it and redistribute it
; freely, subject to the following restrictions:
;
;  1. The origin of this software must not be misrepresented; you must not
;     claim that you wrote the original software. If you use this software in
;     in a product, an acknowledgment in the product documentation would be
;     appreciated but is not required.
;
;  2. Altered source versions must be plainly marked as such, and must not be
;     misrepresented as being the original software.
;
;  3. This notice may not be removed or altered from any source distribution.
; ----------------------------------------------------------------------------

; Definitions from libmc1 (be sure to add libmc1/include to the include path).
.include "mc1/memory.inc"
.include "mc1/mmio.inc"

.ifdef STACK_IN_XRAM
    STACK_SIZE = 512*1024
.else
    STACK_SIZE = 4*1024
.endif

    .section .text.start, "ax"

    .globl  _start
    .p2align 2

_start:
    ; ------------------------------------------------------------------------
    ; Set up the stack. By default the stack is placed at the top of VRAM.
    ; Define STACK_IN_XRAM to use XRAM for the stack.
    ; ------------------------------------------------------------------------

    ldi     r20, #MMIO_START
.ifdef STACK_IN_XRAM
    ldw     r1, [r20, #XRAMSIZE]
    ldi     sp, #XRAM_START
.else
    ldw     r1, [r20, #VRAMSIZE]
    ldi     sp, #VRAM_START
.endif
    add     sp, sp, r1              ; sp = Top of stack


    ; ------------------------------------------------------------------------
    ; Clear the BSS data (if any).
    ; ------------------------------------------------------------------------

    ldi     r2, #__bss_size
    bz      r2, 2f
    lsr     r2, r2, #2      ; BSS size is always a multiple of 4 bytes.

    ldi     r1, #__bss_start
    getsr   vl, #0x10
1:
    minu    vl, vl, r2
    sub     r2, r2, vl
    stw     vz, [r1, #4]
    ldea    r1, [r1, vl*4]
    bnz     r2, 1b
2:


    ; ------------------------------------------------------------------------
    ; Initialize the memory allocator.
    ; ------------------------------------------------------------------------

    call    #mem_init@pc

    ; Add a memory pool for the XRAM (if any).
    ldi     r1, #__xram_free_start

    ldw     r2, [r20, #XRAMSIZE]
    bz      r2, 1f
    ldi     r3, #XRAM_START
    sub     r3, r1, r3
    sub     r2, r2, r3
.ifdef STACK_IN_XRAM
    add     r2, r2, #-STACK_SIZE
.endif

    ldi     r3, #MEM_TYPE_EXT
    call    #mem_add_pool@pc
1:

    ; Add a memory pool for the VRAM.
    ldi     r1, #__vram_free_start

    ldw     r2, [r20, #VRAMSIZE]
    ldi     r3, #VRAM_START
    sub     r3, r1, r3
    sub     r2, r2, r3
.ifndef STACK_IN_XRAM
    add     r2, r2, #-STACK_SIZE
.endif

    ldi     r3, #MEM_TYPE_VIDEO
    call    #mem_add_pool@pc


    ; ------------------------------------------------------------------------
    ; Handle global constructors/destructors (C++).
    ; ------------------------------------------------------------------------

    ; Call _init() to run static constructors.
    call    #_init@pc

    ; Call _fini at exit time for static destructors.
    ldi     r1, #_fini
    call	#atexit@pc


    ; ------------------------------------------------------------------------
    ; Call main().
    ; ------------------------------------------------------------------------

    ; r1 = argc, r2 = argv (these are invalid - don't use them!)
    ldi     r1, #0
    ldi     r2, #0

    ; Jump to main().
    call    #main@pc

    ; Call exit().
    call    #exit@pc


    ; ------------------------------------------------------------------------
    ; Terminate the program: Perform a soft reset.
    ; ------------------------------------------------------------------------

    j       z, #0x0200

