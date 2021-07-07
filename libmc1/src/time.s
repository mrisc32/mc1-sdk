; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MC1 system library: time
; ----------------------------------------------------------------------------

.include "mc1/mmio.inc"

    .text

; ----------------------------------------------------------------------------
; void msleep(int milliseconds)
; ----------------------------------------------------------------------------

    .globl  msleep
    .p2align 2

msleep:
    ble     r1, 3$

    ldi     r3, #MMIO_START
    ldw     r3, [r3, #CPUCLK]
    add     r3, r3, #500
    ldi     r4, #1000
    divu    r3, r3, r4          ; r3 = clock cycles / ms

1$:
    ; This busy loop takes 1 ms on an MRISC32-A1 (2 cycle per iteration).
    lsr     r2, r3, #1
2$:
    add     r2, r2, #-1
    bnz     r2, 2$

    add     r1, r1, #-1
    bnz     r1, 1$

3$:
    ret

