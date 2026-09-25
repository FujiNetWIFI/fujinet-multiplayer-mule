; msxbios.asm -- the BIOS calls the game makes (main ROM, page 0). Each
; takes its one argument in L (__z88dk_fastcall) and answers in L; IX and
; IY are kept.

        SECTION code_user

        PUBLIC  _msx_snsmat, _msx_gtstck, _msx_gttrig, _msx_chsns, _msx_chget

        defc    CHSNS  = $009C  ; Z: no character waiting
        defc    CHGET  = $009F
        defc    GTSTCK = $00D5  ; A stick (0 cursors, 1, 2): A 0, 1 up .. 8 up-left
        defc    GTTRIG = $00D8  ; A trigger (1: stick 1 A): A FFh pressed
        defc    SNSMAT = $0141  ; A row: A its keys, 0 = down

bios:   push    ix
        push    iy
        ld      a,l
        call    jphl_de
        pop     iy
        pop     ix
        ld      l,a
        ret
jphl_de:
        push    de
        ret

_msx_snsmat:
        ld      de,SNSMAT
        jr      bios
_msx_gtstck:
        ld      de,GTSTCK
        jr      bios
_msx_gttrig:
        ld      de,GTTRIG
        jr      bios
_msx_chget:
        ld      de,CHGET
        jr      bios
_msx_chsns:
        push    ix
        push    iy
        call    CHSNS
        pop     iy
        pop     ix
        ld      l,0
        ret     z
        inc     l
        ret
