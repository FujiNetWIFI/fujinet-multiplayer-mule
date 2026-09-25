; msx_rom_header.asm -- the cartridge header (z88dk's rom CRT includes it,
; CRT_MSX_CUSTOM_HEADER=1): "AB" and the start, which first maps the ASCII8
; banks 1, 2, 3 at 6000h, 8000h and A000h (a mapper may start with bank 0
; in all four). Only 4000h-5FFFh is certain until then: this is there.

        defm    "AB"
        defw    mule_init
        defw    0               ; CALL statements
        defw    0               ; device
        defw    0               ; BASIC text
        defs    6

mule_init:
        ld      a,1
        ld      ($6800),a
        inc     a
        ld      ($7000),a
        inc     a
        ld      ($7800),a
        jp      start
