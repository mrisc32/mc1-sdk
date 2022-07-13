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

    VRAM_STACK_SIZE = 4*1024
    XRAM_STACK_SIZE = 512*1024

    .section .text.start, "ax"

    .globl  _start
    .p2align 2

_start:
    ; ------------------------------------------------------------------------
    ; Set up the stack. We use XRAM if we have it, otherwise VRAM. We place
    ; the stack at the top to minimize the risk of collisions with heap
    ; allocations.
    ; ------------------------------------------------------------------------

    ldi     r20, #MMIO_START
    ldw     r1, [r20, #XRAMSIZE]
    bz      r1, 1f
    ldi     sp, #XRAM_START
    b       2f
1:
    ldw     r1, [r20, #VRAMSIZE]
    ldi     sp, #VRAM_START
2:
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

    ; Add a memory pool for the VRAM.
    ldi     r1, #__vram_free_start

    ; Calculate the size of the memory pool.
    ldw     r2, [r20, #VRAMSIZE]
    ldi     r3, #VRAM_START
    sub     r3, r1, r3
    sub     r2, r2, r3

    ; If we have no XRAM, the stack is at the top of the VRAM. Leve some room.
    ldw     r3, [r20, #XRAMSIZE]
    bnz     r3, 1f
    add     r2, r2, #-VRAM_STACK_SIZE
1:

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



; ----------------------------------------------------------------------------
; Memory configuration override for newlib (e.g. malloc/sbrk).
; ----------------------------------------------------------------------------

    .section .text

; ----------------------------------------------------------------------------
; char* _getheapend(char* heap_start)
; ----------------------------------------------------------------------------
    .globl  _getheapend
    .p2align 2
	.type	_getheapend,@function

_getheapend:
    ldi     r2, #MMIO_START
    sltu    r3, r1, #XRAM_START
    bns     r3, 1f

    ; Heap is in VRAM.
    ldw     r1, [r2, #VRAMSIZE]
    add     r1, r1, #VRAM_START

    ; If we have no XRAM, the stack is at the top of the VRAM. Leve some room.
    ldw     r3, [r2, #XRAMSIZE]
    bnz     r3, 2f
    add     r1, r1, #-VRAM_STACK_SIZE
2:
    ret

1:
    ; Heap is in XRAM.
    ldw     r1, [r2, #XRAMSIZE]
    add     r1, r1, #XRAM_START

    ; Leave room for the stack (which must be in XRAM).
    ldi     r2, #XRAM_STACK_SIZE
    sub     r1, r1, r2
    ret

    .size   _getheapend,.-_getheapend

