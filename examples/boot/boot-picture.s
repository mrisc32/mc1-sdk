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
; This is a MC1 boot demo that loads and displays a picture from the boot image.
;--------------------------------------------------------------------------------------------------

    ; ROM routine table offsets.
    DOH = 0
    BLK_READ = 4
    CRC32C = 8
    LZG_DECODE = 12

    ; Picture dimensions.
    PIC_WIDTH = 1200
    PIC_HEIGHT = 675
    PIC_BPP = 2

    ; Picture VRAM addresses.
    PIC_ADDR = 0x40003000
    PIC_STRIDE = (PIC_WIDTH*PIC_BPP)/8

    ; ...in video address space.
    PIC_VADDR = (PIC_ADDR-0x40000000)/4
    PIC_VSTRIDE = PIC_STRIDE/4

    ; ...in the boot image.
    PIC_BLOCK = 1
    PIC_NUM_BLOCKS = 396

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

    ldi     r9, #0x50007fff     ; Wait forever
    stw     r9, [r10, #16]

    ; Generate VCP prologue layer 1.
    addpchi r2, #vcp_preamble@pchi
    add     r2, r2, #vcp_preamble+4@pclo
    add     r3, r10, #32
    ldi     r4, #0
1:
    ldw     r1, [r2, r4*4]
    stw     r1, [r3, r4*4]
    add     r4, r4, #1
    slt     r15, r4, #vcp_preamble_len
    bs      r15, 1b
    ldea    r3, [r3, r4*4]

    ; Generate line addresses.
    ldi     r2, #0x80000000+PIC_VADDR   ; SETREG ADDR, ...
    ldi     r5, #0x50000000             ; WAITY ...
    ldi     r6, #1080
    ldi     r4, #1
2:
    mul     r8, r4, r6
    ldi     r7, #PIC_HEIGHT
    div     r8, r8, r7          ; r8 = line to wait for
    add     r8, r5, r8
    stw     r2, [r3, #0]        ; SETREG ADDR, ...
    stw     r8, [r3, #4]        ; WAITY ...
    add     r2, r2, #PIC_VSTRIDE
    add     r3, r3, #8
    add     r4, r4, #1
    sle     r15, r4, #PIC_HEIGHT
    bs      r15, 2b

    ; VCP epilogue.
    stw     r9, [r3]            ; Wait forever

    ; Clear the frame buffer.
    ldi     r1, #PIC_ADDR
    ldi     r2, #PIC_ADDR+PIC_HEIGHT*PIC_STRIDE
3:
    stw     z, [r1]
    add     r1, r1, #4
    slt     r3, r1, r2
    bs      r3, 3b

    ; Load the picture into the frame buffer.
    ldi     r1, #PIC_ADDR
    ldi     r2, #0
    ldi     r3, #PIC_BLOCK
    ldi     r4, #PIC_NUM_BLOCKS
    jl      r26, #BLK_READ

    ; Loop forever...
4:
    b       4b

    .p2align 2
vcp_preamble:
    .word   0x85000004          ; SETREG CMODE, 4 (PAL2)
    .word   0x60000003          ; SETPAL 0, 4
    .word   0xff000000          ; Color 0
    .word   0xff555555          ; Color 1
    .word   0xffaaaaaa          ; Color 2
    .word   0xffffffff          ; Color 3
    .word   0x82000000+((65536*PIC_WIDTH)/1920)  ; SETREG XINCR, ...
    .word   0x50000000          ; Wait for Y=0
    .word   0x84000780          ; SETREG HSTOP, 1920
vcp_preamble_len = (.-vcp_preamble)/4

