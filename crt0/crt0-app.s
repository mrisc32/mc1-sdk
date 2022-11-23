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


; ----------------------------------------------------------------------------
; Preserved registers (from the boot program).
; ----------------------------------------------------------------------------

    .L_STACK_R1 = 0
    .L_STACK_R2 = 4
    .L_STACK_R16 = 8
    .L_STACK_R17 = 12
    .L_STACK_R18 = 16
    .L_STACK_R19 = 20
    .L_STACK_R20 = 24
    .L_STACK_R21 = 28
    .L_STACK_R22 = 32
    .L_STACK_R23 = 36
    .L_STACK_R24 = 40
    .L_STACK_R25 = 44
    .L_STACK_R26 = 48
    .L_STACK_TP = 52
    .L_STACK_FP = 56
    .L_STACK_SP = 60
    .L_STACK_LR = 64
    .L_STACK_VL = 68
    .L_STACK_SIZE = 72

    ; Note: We can't use BSS here since we zero-fill the entire BSS area
    ; *after* storing the saved registers (i.e they would be over-written).
    ; Thus we store the saved registers in the .data section instead.
    .section .data, "aw"
.L_saved_regs:
    .skip   .L_STACK_SIZE


; ----------------------------------------------------------------------------
; Program entry.
; ----------------------------------------------------------------------------

    .section .text.start, "ax"

    .globl  _start
    .p2align 2

_start:
    ; ------------------------------------------------------------------------
    ; Preserve calle-save registers and argc (R1) + argv (R2).
    ; ------------------------------------------------------------------------

    ldi     r15, #.L_saved_regs@pc
    stw     r1, [r15, #.L_STACK_R1]
    stw     r2, [r15, #.L_STACK_R2]
    stw     r16, [r15, #.L_STACK_R16]
    stw     r17, [r15, #.L_STACK_R17]
    stw     r18, [r15, #.L_STACK_R18]
    stw     r19, [r15, #.L_STACK_R19]
    stw     r20, [r15, #.L_STACK_R20]
    stw     r21, [r15, #.L_STACK_R21]
    stw     r22, [r15, #.L_STACK_R22]
    stw     r23, [r15, #.L_STACK_R23]
    stw     r24, [r15, #.L_STACK_R24]
    stw     r25, [r15, #.L_STACK_R25]
    stw     r26, [r15, #.L_STACK_R26]
    stw     tp, [r15, #.L_STACK_TP]
    stw     fp, [r15, #.L_STACK_FP]
    stw     sp, [r15, #.L_STACK_SP]
    stw     lr, [r15, #.L_STACK_LR]
    stw     vl, [r15, #.L_STACK_VL]


    ; ------------------------------------------------------------------------
    ; Set up the stack. The stack location and size is defined by the linker
    ; script.
    ; ------------------------------------------------------------------------

    ldi     sp, #__stack_end


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
    ; Override the default newlib _exit() method.
    ; ------------------------------------------------------------------------

    addpc   r1, #_mc1_app_exit_handler@pc
    call    #_set_exit_handler@pc


    ; ------------------------------------------------------------------------
    ; Call main().
    ; ------------------------------------------------------------------------

    ; r1 = argc, r2 = argv
    ldi     r15, #.L_saved_regs@pc
    ldw     r1, [r15, #.L_STACK_R1]
    ldw     r2, [r15, #.L_STACK_R2]

    ; Call main().
    call    #main@pc

    ; Jump to exit() (does not return).
    tail    #exit@pc


; ----------------------------------------------------------------------------
; void _mc1_app_exit_handler(int status)
; ----------------------------------------------------------------------------

_mc1_app_exit_handler:
    ; Restore callee-saved registers.
    ldi     r15, #.L_saved_regs@pc
    ldw     r16, [r15, #.L_STACK_R16]
    ldw     r17, [r15, #.L_STACK_R17]
    ldw     r18, [r15, #.L_STACK_R18]
    ldw     r19, [r15, #.L_STACK_R19]
    ldw     r20, [r15, #.L_STACK_R20]
    ldw     r21, [r15, #.L_STACK_R21]
    ldw     r22, [r15, #.L_STACK_R22]
    ldw     r23, [r15, #.L_STACK_R23]
    ldw     r24, [r15, #.L_STACK_R24]
    ldw     r25, [r15, #.L_STACK_R25]
    ldw     r26, [r15, #.L_STACK_R26]
    ldw     tp, [r15, #.L_STACK_TP]
    ldw     fp, [r15, #.L_STACK_FP]
    ldw     sp, [r15, #.L_STACK_SP]
    ldw     lr, [r15, #.L_STACK_LR]
    ldw     vl, [r15, #.L_STACK_VL]

    ; Return to the caller (boot program).
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
    ret

1:
    ; Heap is in XRAM.
    ldw     r1, [r2, #XRAMSIZE]
    add     r1, r1, #XRAM_START
    ret

    .size   _getheapend,.-_getheapend

