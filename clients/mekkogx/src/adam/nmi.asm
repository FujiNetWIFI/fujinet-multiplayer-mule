; nmi.asm -- the Adam's frame clock: the VDP's interrupt is the Z80's NMI
; (the CRT jumps here from 0066h: CRT_ENABLE_NMI=2 in the Makefile).
;
; Reading the VDP's status acknowledges the interrupt, but also resets its
; address latch, which would spoil an address being set or a burst of data
; under way. While the game is at the VDP (vdp_busy, see
; src/tms9918/vdp.h) the status is left and vdp_pend set; the VDP holds its
; interrupt line down meanwhile, so no other NMI comes, and VDP_UNLOCK
; catches up (plat.c vdp_catch_up).

        SECTION code_user

        PUBLIC  _z80_nmi
        EXTERN  _vdp_busy, _vdp_pend, _vdp_seen, _frame_count

_z80_nmi:
        push    af
        ld      a,(_vdp_busy)
        or      a
        jr      nz,busy
        in      a,($BF)         ; the status: acknowledges it
        push    hl
        ld      hl,_vdp_seen
        or      (hl)
        ld      (hl),a
        ld      hl,(_frame_count)
        inc     hl
        ld      (_frame_count),hl
        pop     hl
        pop     af
        retn
busy:
        ld      a,1
        ld      (_vdp_pend),a
        pop     af
        retn
