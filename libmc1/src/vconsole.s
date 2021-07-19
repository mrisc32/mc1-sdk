; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
; ----------------------------------------------------------------------------
; MC1 system library: Video console output
; ----------------------------------------------------------------------------

.include "mc1/memory.inc"
.include "mc1/mmio.inc"

VCON_COLS = 80
VCON_ROWS = 45

VCON_WIDTH  = VCON_COLS*8
VCON_HEIGHT = VCON_ROWS*8

VCON_VCP_SIZE = (8 + VCON_HEIGHT*2) * 4
VCON_FB_SIZE  = (VCON_WIDTH * VCON_HEIGHT) / 8

VCON_COL0 = 0x009b2c2e
VCON_COL1 = 0xffeb6d70


    .lcomm  vcon_vcp_start, 4
    .lcomm  vcon_pal_start, 4
    .lcomm  vcon_fb_start, 4

    .lcomm  vcon_col, 4
    .lcomm  vcon_row, 4

    .text

; ----------------------------------------------------------------------------
; unsigned vcon_memory_requirement(void)
; Determine the memory requirement for the video console.
; ----------------------------------------------------------------------------

    .globl  vcon_memory_requirement
    .p2align 2

vcon_memory_requirement:
    ldi     r1, #VCON_VCP_SIZE+VCON_FB_SIZE
    ret


; ----------------------------------------------------------------------------
; void vcon_init(void* addr)
; Create and activate a VCP, a text buffer and a frame buffer.
; ----------------------------------------------------------------------------

    .globl  vcon_init
    .p2align 2

vcon_init:
    add     sp, sp, #-4
    stw     lr, [sp, #0]

    ; Get the native resolution of the video logic.
    ldi     r2, #MMIO_START
    ldw     r3, [r2, #VIDWIDTH]         ; r3 = native video width (e.g. 1920)
    ldw     r4, [r2, #VIDHEIGHT]        ; r4 = native video height (e.g. 1080)

    ; Calculate the VCP and FB base addresses.
    ; r1 = VCP base address
    ldi     r7, #vcon_vcp_start@hi
    stw     r1, [r7, #vcon_vcp_start@lo]

    add     r6, r1, #VCON_VCP_SIZE      ; r6 = FB base address
    ldi     r7, #vcon_fb_start@hi
    stw     r6, [r7, #vcon_fb_start@lo]

    ldi     r7, #VRAM_START
    sub     r6, r6, r7
    lsr     r6, r6, #2                  ; r6 = FB base in video address space

    ; Generate the VCP: Prologue.
    ldi     r8, #0x010000*VCON_WIDTH
    div     r8, r8, r3                  ; r8 = (0x010000 * VCON_WIDTH) / native width
    ldi     r7, #0x82000000
    or      r7, r7, r8
    stw     r7, [r1, #0]                ; SETREG  XINCR, ...

    ldi     r7, #0x84000000
    or      r7, r7, r3
    stw     r7, [r1, #4]                ; SETREG  HSTOP, native width

    ldi     r7, #0x85000005
    stw     r7, [r1, #8]                ; SETREG  CMODE, 5

    ldi     r7, #0x86000000
    stw     r7, [r1, #12]               ; SETREG  RMODE, 0  (no dithering)

    ldi     r7, #0x60000001
    stw     r7, [r1, #16]               ; SETPAL  0, 2

    ldi     r7, #VCON_COL0
    stw     r7, [r1, #20]               ; COLOR 0

    ldi     r7, #VCON_COL1
    stw     r7, [r1, #24]               ; COLOR 1

    add     r7, r1, #20
    ldi     r8, #vcon_pal_start@hi
    stw     r7, [r8, #vcon_pal_start@lo] ; Store the palette address

    add     r1, r1, #28

    ; Generate the VCP: Per row memory pointers.
    ldi     r7, #0x80000000
    ldi     r8, #0x50000000
    ldi     r11, #VCON_HEIGHT
    ldi     r9, #0
1$:
    mul     r10, r9, r4
    div     r10, r10, r11               ; r10 = y * native_height / VCON_HEIGHT
    or      r10, r8, r10
    stw     r10, [r1, #0]               ; WAITY   y * native_height / VCON_HEIGHT
    add     r9, r9, #1

    add     r10, r7, r6
    stw     r10, [r1, #4]               ; SETREG  ADDR, ...
    add     r6, r6, #VCON_COLS/4

    add     r1, r1, #8

    seq     r10, r9, #VCON_HEIGHT
    bns     r10, 1$

    ; Generate the VCP: Epilogue.
    ldi     r7, #0x50007fff
    stw     r7, [r1, #0]                ; WAITY  32767

    ; Clear the screen.
    bl      vcon_clear

    ; Activate the vconsole VCP.
    ldi     r2, #1                      ; LAYER_1
    bl      vcon_show

    ldw     lr, [sp, #0]
    add     sp, sp, #4
    ret


; ----------------------------------------------------------------------------
; void vcon_show(layer_t layer)
; Activate the vcon VCP.
; ----------------------------------------------------------------------------

    .globl  vcon_show
    .p2align 2

vcon_show:
    ; Valid layer (i.e. in the range [1, 2])?
    add     r2, r1, #-1
    sleu    r2, r2, #1
    bns     r2, 1$

    ; Get the VCP start address.
    ldi     r2, #vcon_vcp_start@hi
    ldw     r2, [r2, #vcon_vcp_start@lo]
    bz      r2, 1$

    ; Convert the address to the VCP address space.
    ldi     r3, #VRAM_START
    sub     r2, r2, r3
    lsr     r2, r2, #2          ; r2 = (vcon_vcp_start - VRAM_START) / 4

    ; Emit a JMP instruction for the selected layer.
    lsl     r1, r1, #4          ; Layer VCP start = VRAM_START + layer * 16
    stw     r2, [r3, r1]
1$:
    ret


; ----------------------------------------------------------------------------
; void vcon_clear()
; Clear the VCON frame buffer and reset the coordinates.
; ----------------------------------------------------------------------------

    .globl  vcon_clear
    .p2align 2

vcon_clear:
    ; Clear the col, row coordinate.
    ldi     r1, #vcon_col@hi
    stw     z, [r1, #vcon_col@lo]
    ldi     r1, #vcon_row@hi
    stw     z, [r1, #vcon_row@lo]

    ; Clear the frame buffer.
    ldi     r1, #vcon_fb_start@hi
    ldw     r1, [r1, #vcon_fb_start@lo]
    ldi     r2, #0
    ldi     r3, #VCON_FB_SIZE

    b       memset


; ----------------------------------------------------------------------------
; void vcon_set_colors(unsigned col0, unsigned col1)
; Set the palette.
; ----------------------------------------------------------------------------

    .globl  vcon_set_colors
    .p2align 2

vcon_set_colors:
    ldi     r3, #vcon_pal_start@hi
    ldw     r3, [r3, #vcon_pal_start@lo]
    stw     r1, [r3, #0]
    stw     r2, [r3, #4]

    ret


; ----------------------------------------------------------------------------
; void vcon_print(char* text)
; Print a zero-terminated string.
; ----------------------------------------------------------------------------

    .globl  vcon_print
    .p2align 2

vcon_print:
    mov     r10, vl                     ; Preserve vl (without using the stack)

    ldi     r2, #mc1_font_8x8@pc        ; r2 = font

    ldi     r3, #vcon_col@hi
    ldw     r3, [r3, #vcon_col@lo]      ; r3 = col

    ldi     r4, #vcon_row@hi
    ldw     r4, [r4, #vcon_row@lo]      ; r4 = row

    ldi     r8, #vcon_fb_start@hi
    ldw     r8, [r8, #vcon_fb_start@lo] ; r8 = frame buffer start

1$:
    ldub    r5, [r1]
    add     r1, r1, #1
    bz      r5, 2$

    ; New line (LF)?
    seq     r6, r5, #10
    bs      r6, 3$

    ; Carriage return (CR)?
    seq     r6, r5, #13
    bns     r6, 4$
    ldi     r3, #0
    b       1$

4$:
    ; Tab?
    seq     r6, r5, #9
    bns     r6, 5$
    add     r3, r3, #8
    and     r3, r3, #~7
    slt     r6, r3, #VCON_COLS
    bs      r6, 1$
    b       3$

5$:
    ; Printable char.
    max     r5, r5, #32
    min     r5, r5, #127

    add     r5, r5, #-32
    ldea    r5, [r2, r5*8]              ; r5 = start of glyph

    ; Copy glyph (8 bytes) from the font to the frame buffer.
    ldi     vl, #8
    ldi     r7, #VCON_COLS
    mul     r6, r4, r7
    ldub    v1, [r5, #1]                ; Load entire glyph (8 bytes)
    ldea    r6, [r3, r6*8]
    add     r6, r8, r6                  ; r6 = FB + col + (row * VCON_COLS * 8)
    stb     v1, [r6, r7]                ; Store glyph with stride = VCON_COLS

    add     r3, r3, #1
    slt     r5, r3, #VCON_COLS
    bs      r5, 1$

3$:
    ; New line
    ldi     r3, #0
    add     r4, r4, #1
    slt     r5, r4, #VCON_ROWS
    bs      r5, 1$

    ; End of frame buffer.
    ldi     r4, #VCON_ROWS-1

    ; Scroll screen up one row.

    ; 1) Move entire frame buffer.
    ; Clobbered registers: r5, r6, r7, r9
    add     r7, r8, #VCON_COLS*8        ; r7 = source (start of FB + one row)
    mov     r9, r8                      ; r9 = target (start of FB)
    cpuid   r5, z, z
    ldi     r6, #(VCON_COLS*8 * (VCON_ROWS-1)) / 4  ; Number of words to move
6$:
    min     vl, r5, r6
    sub     r6, r6, vl
    ldw     v1, [r7, #4]
    ldea    r7, [r7, vl*4]
    stw     v1, [r9, #4]
    ldea    r9, [r9, vl*4]
    bnz     r6, 6$

    ; 2) Clear last row (continue writing at r9 and forward).
    ldi     r6, #(VCON_COLS*8) / 4      ; Number of words to clear
7$:
    min     vl, r5, r6
    sub     r6, r6, vl
    stw     vz, [r9, #4]
    ldea    r9, [r9, vl*4]
    bnz     r6, 7$

    b       1$

2$:
    ldi     r5, #vcon_col@hi
    stw     r3, [r5, #vcon_col@lo]

    ldi     r5, #vcon_row@hi
    stw     r4, [r5, #vcon_row@lo]

    mov     vl, r10                     ; Restore vl
    ret


; ----------------------------------------------------------------------------
; void vcon_print_hex(unsigned x)
; Print a hexadecimal number.
; ----------------------------------------------------------------------------

    .globl  vcon_print_hex
    .p2align 2

vcon_print_hex:
    add     sp, sp, #-16
    stw     lr, [sp, #12]

    ; Build an ASCII string on the stack.
    ldi     r4, #hex_to_ascii@pc
    ldi     r2, #8
    stb     z, [sp, r2]         ; Zero termination
1$:
    and     r3, r1, #0x0f
    ldub    r3, [r4, r3]
    add     r2, r2, #-1
    lsr     r1, r1, #4
    stb     r3, [sp, r2]
    bgt     r2, 1$

    ; Call the regular printing routine.
    mov     r1, sp
    bl      vcon_print

    ldw     lr, [sp, #12]
    add     sp, sp, #16
    ret


; ----------------------------------------------------------------------------
; void vcon_print_dec(int x)
; Print a signed decimal number.
; ----------------------------------------------------------------------------

    .globl  vcon_print_dec
    .p2align 2

vcon_print_dec:
    add     sp, sp, #-16
    stw     lr, [sp, #12]

    ; Build an ASCII string on the stack.
    ldi     r4, #hex_to_ascii@pc
    ldi     r2, #11
    stb     z, [sp, r2]         ; Zero termination

    ldi     r6, #10

    ; Negative?
    slt     r5, r1, z
    bns     r5, 1$
    sub     r1, z, r1

1$:
    remu    r3, r1, r6
    add     r2, r2, #-1
    ldub    r3, [r4, r3]
    divu    r1, r1, r6
    stb     r3, [sp, r2]
    bnz     r1, 1$

    ; Prepend a minus sign?
    bns     r5, 2$
    ldi     r3, #45             ; Minus sign (-)
    add     r2, r2, #-1
    stb     r3, [sp, r2]

2$:
    ; Call the regular printing routine.
    ldea    r1, [sp, r2]
    bl      vcon_print

    ldw     lr, [sp, #12]
    add     sp, sp, #16
    ret


; ----------------------------------------------------------------------------
; int vcon_putc(const int c)
; Print a single ASCII character.
; ----------------------------------------------------------------------------

    .globl  vcon_putc
    .p2align 2

vcon_putc:
    add     sp, sp, #-12
    stw     lr, [sp, #4]
    stw     r1, [sp, #8]

    ; Store the character as a string on the stack and call vcon_print.
    stb     r1, [sp, #0]
    stb     z, [sp, #1]
    mov     r1, sp
    bl      vcon_print

    ldw     lr, [sp, #4]
    ldw     r1, [sp, #8]
    add     sp, sp, #12
    ret


    .section .rodata
hex_to_ascii:
    .ascii  "0123456789ABCDEF"

