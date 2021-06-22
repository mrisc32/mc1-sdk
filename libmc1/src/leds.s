; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MC1 system library: leds
; ----------------------------------------------------------------------------

.include "mc1/mmio.inc"

    .text

; ----------------------------------------------------------------------------
; void set_leds(unsigned bits)
; Set the leds of the boards.
; ----------------------------------------------------------------------------

    .globl set_leds
    .p2align 2

set_leds:
    ldhi    r2, #MMIO_START
    stw     r1, r2, #LEDS
    ret


; ----------------------------------------------------------------------------
; void sevseg_print_hex(unsigned number)
; Print a hexadecimal number to the board segment displays.
; ----------------------------------------------------------------------------

    .globl  sevseg_print_hex
    .p2align 2

sevseg_print_hex:
    ldi     r2, #glyph_lut@pc
    ldi     r3, #MMIO_START+SEGDISP0

    ldi     r5, #8
1$:
    and     r4, r1, #0x0f
    lsr     r1, r1, #4
    ldub    r4, r2, r4
    stw     r4, r3, #0
    add     r3, r3, #4
    add     r5, r5, #-1
    bnz     r5, 1$

    ret


; ----------------------------------------------------------------------------
; void sevseg_print_dec(int number)
; Print a decimal number to the board segment displays.
; ----------------------------------------------------------------------------

    .globl  sevseg_print_dec
    .p2align 2

sevseg_print_dec:
    ldi     r2, #glyph_lut@pc
    ldi     r3, #MMIO_START+SEGDISP0

    ; Determine the sign of the number.
    slt     r7, r1, z
    bns     r7, 4$
    sub     r1, z, r1
    ldi     r7, #0b1000000  ; r7 = "-" if r1 is negative
4$:

    ldi     r6, #10
    ldi     r5, #8
1$:
    remu    r4, r1, r6      ; r4 = 0..9
    divu    r1, r1, r6
    ldub    r4, r2, r4
2$:
    stw     r4, r3, #0
    add     r3, r3, #4
    add     r5, r5, #-1
    bz      r5, 3$

    bnz     r1, 1$          ; Print more digits as long as the remainder != 0

    mov     r4, r7          ; Leftmost character is the sign (" " or "-").
    ldi     r7, #0          ; Blank the upper digits.
    b       2$

3$:
    ret


; ----------------------------------------------------------------------------
; void sevseg_print(const char* text)
; Print a decimal number to the board segment displays.
; ----------------------------------------------------------------------------

    .globl  sevseg_print
    .p2align 2

sevseg_print:
    ldi     r2, #glyph_lut@pc
    ldi     r3, #MMIO_START+SEGDISP0
    ldi     r7, #alpha_to_glyph_lut@pc

1$:
    ; Get next char.
    ldub    r4, r1, #0
    add     r1, r1, #1
    ldi     r5, #0
    bz      r4, 3$

    slt     r6, r4, #48
    bs      r6, 2$
    slt     r6, r4, #58
    bns     r6, 6$
    ; It's a numeric glyph.
    add     r4, r4, #-48
    b       7$

6$:
    slt     r6, r4, #65
    bs      r6, 2$
    slt     r6, r4, #91
    bns     r6, 2$
    ; It's an alpha glyph.
    add     r4, r4, #-65
    ldub    r4, r7, r4

7$:
    ; Get glyph.
    ldub    r5, r2, r4

    ; Print glyph.
2$:
    stw     r5, r3, #0
    add     r3, r3, #4      ; TODO(m): We should reverse the order...

    b       1$

3$:
    ret


; ----------------------------------------------------------------------------
; 7-segment display bit encoding:
;
;              -0-
;             5   1
;             |-6-|
;             4   2
;              -3-
; ----------------------------------------------------------------------------

    .section .rodata

glyph_lut:
    .byte   0b0111111   ; 0, O
    .byte   0b0000110   ; 1, I
    .byte   0b1011011   ; 2, Z
    .byte   0b1001111   ; 3
    .byte   0b1100110   ; 4
    .byte   0b1101101   ; 5, S
    .byte   0b1111101   ; 6
    .byte   0b0000111   ; 7
    .byte   0b1111111   ; 8
    .byte   0b1101111   ; 9, g
    .byte   0b1110111   ; A
    .byte   0b1111100   ; b
    .byte   0b0111001   ; C
    .byte   0b1011110   ; d
    .byte   0b1111001   ; E
    .byte   0b1110001   ; F

    .byte   0b1110110   ; H
    .byte   0b0001110   ; J
    .byte   0b0111000   ; L
    .byte   0b1110011   ; P
    .byte   0b0110001   ; T
    .byte   0b0111110   ; U, V
    .byte   0b1110010   ; Y

    .byte   0b0000000   ; Space (and unprintable)


alpha_to_glyph_lut:
          ; A   b   C   d   E   F   g  H   I  J   K   L   M   N   O  P   Q
    .byte   10, 11, 12, 13, 14, 15, 9, 16, 1, 17, 23, 18, 23, 23, 0, 19, 23

          ; R   S  T   U   V   W   X   Y   Z
    .byte   23, 5, 20, 21, 21, 23, 23, 22, 2

