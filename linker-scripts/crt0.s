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

    ; Stack configuration.
    .L_VRAM_STACK_SIZE = 4*1024
    .L_XRAM_STACK_SIZE = 512*1024


; ----------------------------------------------------------------------------
; Transitional stack (placed in BSS).
; ----------------------------------------------------------------------------

    .L_STACK_R1 = 0
    .L_STACK_R2 = 4
    .L_STACK_R16 = 8
    .L_STACK_SP = 12
    .L_STACK_LR = 16
    .L_STACK_VL = 20
    .L_STACK_SIZE = 24

    .lcomm  .L_transitional_stack, .L_STACK_SIZE


; ----------------------------------------------------------------------------
; Program entry.
; ----------------------------------------------------------------------------

    .section .text.start, "ax"

    .globl  _start
    .p2align 2

_start:
    ; ------------------------------------------------------------------------
    ; Preserve clobbered calle-save registers and argc (R1) + argv (R2). We
    ; can't use the stack (we don't have one yet) so store the registers in
    ; our BSS area.
    ; ------------------------------------------------------------------------

    ldi     r3, #.L_transitional_stack@pc
    stw     r1, [r3, #.L_STACK_R1]
    stw     r2, [r3, #.L_STACK_R2]
    stw     r16, [r3, #.L_STACK_R16]
    stw     sp, [r3, #.L_STACK_SP]
    stw     lr, [r3, #.L_STACK_LR]
    stw     vl, [r3, #.L_STACK_VL]


    ; ------------------------------------------------------------------------
    ; Set up the stack. We use XRAM if we have it, otherwise VRAM. We place
    ; the stack at the top to minimize the risk of collisions with heap
    ; allocations.
    ; TODO(m): Use BSS instead to avoid heap allocation collisions and
    ; simplify logic?
    ; ------------------------------------------------------------------------

    ldi     r16, #MMIO_START
    ldw     r1, [r16, #XRAMSIZE]
    bz      r1, 1f
    ldi     sp, #XRAM_START
    b       2f
1:
    ldw     r1, [r16, #VRAMSIZE]
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

    ; r1 = argc, r2 = argv
    ldi     r3, #.L_transitional_stack@pc
    ldw     r1, [r3, #.L_STACK_R1]
    ldw     r2, [r3, #.L_STACK_R2]

    ; Call main().
    call    #main@pc
    mov     r16, r1     ; Preserve return value.

    ; Call exit().
    call    #exit@pc


    ; ------------------------------------------------------------------------
    ; Return to the caller (if we got this far).
    ; ------------------------------------------------------------------------

    ; Return value from main() goes in R1.
    mov     r1, r16

    ; Restore callee-saved registers.
    ldi     r3, #.L_transitional_stack@pc
    ldw     r16, [r3, #.L_STACK_R16]
    ldw     sp, [r3, #.L_STACK_SP]
    ldw     lr, [r3, #.L_STACK_LR]
    ldw     vl, [r3, #.L_STACK_VL]

    ret


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
    add     r1, r1, #-.L_VRAM_STACK_SIZE
2:
    ret

1:
    ; Heap is in XRAM.
    ldw     r1, [r2, #XRAMSIZE]
    add     r1, r1, #XRAM_START

    ; Leave room for the stack (which must be in XRAM).
    ldi     r2, #.L_XRAM_STACK_SIZE
    sub     r1, r1, r2
    ret

    .size   _getheapend,.-_getheapend

