;
; exehdr.s -- the .lnx header emulators and flash carts read (in place of
; cc65's, which names every cart "Cart name"). After Karri Kaksonen's.
;
        .import         __BANK0BLOCKSIZE__
        .import         __BANK1BLOCKSIZE__
        .export         __EXEHDR__: absolute = 1

        .segment "EXEHDR"
        .byte   'L','Y','N','X'                 ; magic
        .word   __BANK0BLOCKSIZE__              ; bank 0 page size
        .word   __BANK1BLOCKSIZE__              ; bank 1 page size
        .word   1                               ; version
        .byte   "M.U.L.E. Tournament"           ; 32 bytes: the name
        .res    32 - 19, 0
        .byte   "FujiNet"                       ; 16 bytes: the maker
        .res    16 - 7, 0
        .byte   0                               ; rotation: none
        .byte   0,0,0,0,0                       ; spare
