; msxint.asm -- the MSX's frame clock and the way out.
;
; The BIOS's interrupt (the VDP's, 60 or 50 a second) reads the VDP's
; status and calls the H.TIMI hook with it in A: the hook counts a frame,
; keeps the status bits (vdp_seen: the fifth sprite flag) and goes on to
; whatever the hook did before (a disk ROM's timers).
;
; msx_quit: back to the FujiNet's own ROM. Its user ROM (us) is switched
; off by the write to its control register, so that is done from RAM, then
; the machine restarts.

        SECTION code_user

        PUBLIC  _msx_hook_init, _msx_quit
        EXTERN  _frame_count, _vdp_seen

        defc    H_TIMI = $FD9F
        defc    IO_CONTROL = $BFFF

_msx_hook_init:
        di
        ld      hl,H_TIMI
        ld      de,old_hook
        ld      bc,5
        ldir
        ld      a,$C3           ; jp timi
        ld      (H_TIMI),a
        ld      hl,timi
        ld      (H_TIMI+1),hl
        ei
        ret

timi:
        push    af
        push    hl
        ld      hl,_vdp_seen
        or      (hl)
        ld      (hl),a
        ld      hl,(_frame_count)
        inc     hl
        ld      (_frame_count),hl
        pop     hl
        pop     af
        jp      old_hook

_msx_quit:
        di
        ld      hl,H_TIMI       ; the hook as it was
        ex      de,hl
        ld      hl,old_hook
        ld      bc,5
        ldir
        ld      hl,quit_stub
        ld      de,quit_ram
        ld      bc,quit_end - quit_stub
        ldir
        jp      quit_ram

quit_stub:
        ld      a,$06           ; ROM mode: the FujiNet's ROM on, the user ROM off
        ld      (IO_CONTROL),a
        rst     0
quit_end:

        SECTION bss_user
old_hook:       defs    5
quit_ram:       defs    8
