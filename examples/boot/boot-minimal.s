; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
;--------------------------------------------------------------------------------------------------
; Copyright (c) 2021 Marcus Geelnard
;
; This software is provided 'as-is', without any express or implied warranty. In no event will the
; authors be held liable for any damages arising from the use of this software.
;
; Permission is granted to anyone to use this software for any purpose, including commercial
; applications, and to alter it and redistribute it freely, subject to the following restrictions:
;
;  1. The origin of this software must not be misrepresented; you must not claim that you wrote
;     the original software. If you use this software in a product, an acknowledgment in the
;     product documentation would be appreciated but is not required.
;
;  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
;     being the original software.
;
;  3. This notice may not be removed or altered from any source distribution.
;--------------------------------------------------------------------------------------------------

;--------------------------------------------------------------------------------------------------
; This is a minimal MC1 boot routine that displays a pattern on the screen. It can be used for
; testing that the MC1 boot process works properly.
;--------------------------------------------------------------------------------------------------

    .section .text.start, "ax"
    .globl  _boot
    .p2align 2

;--------------------------------------------------------------------------------------------------
; void _boot(const void* rom_base);
;  r1 = rom_base
;--------------------------------------------------------------------------------------------------
_boot:
    ; Store the ROM jump table address in r26.
    mov     r26, r1

    ; r10 = start of VRAM
    ldi     r10, #0x40000000

    ; Configure VCP for layer 1 to be silent.
    ldi     r1, #0x50007fff     ; Wait forever
    stw     r1, [r10, #16]

    ; Configure VCP for layer 2 to jump to our VCP.
    ldi     r1, #vcp@pc
    sub     r1, r1, r10
    lsr     r1, r1, #2          ; r1 = JMP vcp (in video address space)
    stw     r1, [r10, #32]

    ; Define the frame buffer (just a horizontal bit pattern).
    ldi     r1, #0x55555555
    stw     r1, [r10, #64]      ; Frame buffer @ 0x40000040

    ; Loop forever.
1:
    b       1b

    .p2align 2
vcp:
    .word   0x85000005          ; SETREG CMODE, 5 (PAL1)
    .word   0x60000001          ; SETPAL 0, 2
    .word   0xff80ff80          ; Color 0
    .word   0xff600020          ; Color 1
    .word   0x82000422          ; SETREG XINCR, 0x00.0422 (31/1920)
    .word   0x50000000          ; Wait for Y=0
    .word   0x84000780          ; SETREG HSTOP, 1920
    .word   0x80000010          ; SETREG ADDR, 0x000010 (0x40000040)
    .word   0x50000000+68       ; Wait for Y=68
    .word   0x81010000          ; SETREG XOFFS, 0x01.0000
    .word   0x50000000+68*2     ; Wait for Y=68*2
    .word   0x81000000          ; SETREG XOFFS, 0x00.0000
    .word   0x50000000+68*3     ; Wait for Y=68*3
    .word   0x81010000          ; SETREG XOFFS, 0x01.0000
    .word   0x50000000+68*4     ; Wait for Y=68*4
    .word   0x81000000          ; SETREG XOFFS, 0x00.0000
    .word   0x50000000+68*5     ; Wait for Y=68*5
    .word   0x81010000          ; SETREG XOFFS, 0x01.0000
    .word   0x50000000+68*6     ; Wait for Y=68*6
    .word   0x81000000          ; SETREG XOFFS, 0x00.0000
    .word   0x50000000+68*7     ; Wait for Y=68*7
    .word   0x81010000          ; SETREG XOFFS, 0x01.0000
    .word   0x50000000+68*8     ; Wait for Y=68*8
    .word   0x81000000          ; SETREG XOFFS, 0x00.0000
    .word   0x50000000+68*9     ; Wait for Y=68*9
    .word   0x81010000          ; SETREG XOFFS, 0x01.0000
    .word   0x50000000+68*10    ; Wait for Y=68*10
    .word   0x81000000          ; SETREG XOFFS, 0x00.0000
    .word   0x50000000+68*11    ; Wait for Y=68*11
    .word   0x81010000          ; SETREG XOFFS, 0x01.0000
    .word   0x50000000+68*12    ; Wait for Y=68*12
    .word   0x81000000          ; SETREG XOFFS, 0x00.0000
    .word   0x50000000+68*13    ; Wait for Y=68*13
    .word   0x81010000          ; SETREG XOFFS, 0x01.0000
    .word   0x50000000+68*14    ; Wait for Y=68*14
    .word   0x81000000          ; SETREG XOFFS, 0x00.0000
    .word   0x50000000+68*15    ; Wait for Y=68*15
    .word   0x81010000          ; SETREG XOFFS, 0x01.0000
    .word   0x50007fff          ; Wait forever

