; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; This file contains the startup code for a secondary boot program. It
; defines _start, which does some initialization and then calls main.
; ----------------------------------------------------------------------------
; Copyright (c) 2022 Marcus Geelnard
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


; ----------------------------------------------------------------------------
; Allocate the stack in the BSS section.
; ----------------------------------------------------------------------------

    .L_STACK_SIZE = 512

    .lcomm .L_stack, .L_STACK_SIZE


; ----------------------------------------------------------------------------
; Program entry.
; ----------------------------------------------------------------------------

    .section .text.start, "ax"

    .globl  _start
    .p2align 2

_start:
    ; ------------------------------------------------------------------------
    ; Set the stack pointer.
    ; ------------------------------------------------------------------------

    addpc   sp, #.L_stack+.L_STACK_SIZE@pc

    ; ------------------------------------------------------------------------
    ; Clear the BSS data (if any).
    ; ------------------------------------------------------------------------

    addpc   r1, #__bss_start@pc
    ldi     r2, #0
    ldi     r3, #__bss_size
    bl      #memset@pc

    ; ------------------------------------------------------------------------
    ; NOTE: We don't support global constructors/destructors (C++). If we did
    ; we would call _init() here and set up _fini() as an atexit() routine.
    ; ------------------------------------------------------------------------

    ; ------------------------------------------------------------------------
    ; Call main().
    ; ------------------------------------------------------------------------

    ; r1 = argc, r2 = argv (these are invalid - don't use them!)
    ldi     r1, #0
    ldi     r2, #0

    ; Call main().
    bl      #main@pc

    ; If the boot program returned, do a soft reset (into ROM).
    j       z, #0x00000200
