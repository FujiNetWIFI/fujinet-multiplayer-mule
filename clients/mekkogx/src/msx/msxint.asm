; msxint.asm -- the MSX's frame clock and the way out.
;
; The BIOS's interrupt (the VDP's, 60 or 50 a second) reads the VDP's
; status and calls the H.TIMI hook with it in A: the hook counts a frame,
; keeps the status bits (vdp_seen: the fifth sprite flag) and goes on to
; whatever the hook did before (a disk ROM's timers). The count is in
; 60ths of a second, as the server's: a 50 Hz machine (bit 7 of the BIOS's
; ID byte at $002B) counts every fifth interrupt twice, as the Atari does
; on PAL. Otherwise main.c's clock_sync put ten a second back, in a lurch
; at every reply.
;
; msx_quit: back to the FujiNet's own ROM. Its user ROM (us) is switched
; off by the write to its control register, so that is done from RAM, then
; the machine restarts.

        SECTION code_user

        PUBLIC  _msx_hook_init, _msx_quit
        EXTERN  _frame_count, _vdp_seen

        defc    H_TIMI = $FD9F
        defc    MSXID1 = $002B          ; bit 7: 50 Hz
        defc    IO_CONTROL = $BFFF

_msx_hook_init:
        di
        ld      a,(MSXID1)
        and     $80
        ld      (hz50),a
        ld      a,5
        ld      (hz50_n),a
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
        ld      a,(hz50)
        or      a
        jr      z,timi_n
        ld      a,(hz50_n)
        dec     a
        jr      nz,timi_5
        inc     hl              ; the fifth 50th: two 60ths
        ld      a,5
timi_5:
        ld      (hz50_n),a
timi_n:
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
hz50:           defs    1
hz50_n:         defs    1
quit_ram:       defs    8
