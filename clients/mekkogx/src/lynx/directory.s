;
; directory.s -- the cart's directory: the program (entry 0), then the
; overlays OVL1..5 (entries 1..5), each where the one before it ends in the
; image (mule-lynx.cfg). lynx_load(n) reads entry n into its address
; (src/lynx/bank.c). After cc65's defdir.s (Karri Kaksonen).
;
        .import         __STARTOFDIRECTORY__, __BANK0BLOCKSIZE__
        .import         __MAIN_START__, __STARTUP_LOAD__, __BSS_LOAD__
        .import         __OVERLAY1_LOAD__, __OVERLAY1_SIZE__
        .import         __OVERLAY2_LOAD__, __OVERLAY2_SIZE__
        .import         __OVERLAY3_LOAD__, __OVERLAY3_SIZE__
        .import         __OVERLAY4_LOAD__, __OVERLAY4_SIZE__
        .import         __OVERLAY5_LOAD__, __OVERLAY5_SIZE__

.macro  entry   off, addr, len
        .byte   <((off) / __BANK0BLOCKSIZE__)
        .word   (off) & (__BANK0BLOCKSIZE__ - 1)
        .byte   $88
        .word   addr
        .word   len
.endmacro

        .segment "DIRECTORY"

dir_start:
off0 = __STARTOFDIRECTORY__ + (dir_end - dir_start)
len0 = __BSS_LOAD__ - __STARTUP_LOAD__
off1 = off0 + len0
off2 = off1 + __OVERLAY1_SIZE__
off3 = off2 + __OVERLAY2_SIZE__
off4 = off3 + __OVERLAY3_SIZE__
off5 = off4 + __OVERLAY4_SIZE__
        entry   off0, __MAIN_START__, len0
        entry   off1, __OVERLAY1_LOAD__, __OVERLAY1_SIZE__
        entry   off2, __OVERLAY2_LOAD__, __OVERLAY2_SIZE__
        entry   off3, __OVERLAY3_LOAD__, __OVERLAY3_SIZE__
        entry   off4, __OVERLAY4_LOAD__, __OVERLAY4_SIZE__
        entry   off5, __OVERLAY5_LOAD__, __OVERLAY5_SIZE__
dir_end:
