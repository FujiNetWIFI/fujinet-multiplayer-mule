; smsint.asm -- the SMS's frame clock and its sound.
;
; z88dk's crt0 takes the VDP's interrupt (IM 1, 0038h): it reads the status
; (__tms9918_status_register) and calls the handlers in raster_procs on a
; frame's; sms_frame is the first. It counts the frame, keeps the status
; bits (vdp_seen: bit 6, a ninth sprite on a line) and times the sound
; effect (snd_frame, sound.c). The count is in 60ths of a second, as the
; server's: on a 50 Hz console (sms_pal, set by plat.c) every fifth frame
; counts twice, as on the MSX and the Atari.
;
; Pause is the Z80's NMI: the crt0 flips _pause_flag (input.c looks).
;
; A handler may change AF and HL only (the crt0 keeps the rest of them).

        SECTION code_user

        PUBLIC  _sms_int_init, _sms_pal_probe, _sms_pal, _snd_frame
        EXTERN  raster_procs, __tms9918_status_register
        EXTERN  _frame_count, _vdp_seen
        EXTERN  _snd_f, _snd_nf, _snd_n

        defc    PSG = $7F

_sms_int_init:
        di
        ld      hl,sms_frame
        ld      (raster_procs),hl
        ld      a,5
        ld      (hz50_n),a
        ei
        ret

; 1 (in L) on a 50 Hz console: a frame there is 71364 Z80 cycles, on a
; 60 Hz one 59736, so the passes of a 25-cycle loop between two frames
; are about 2850 or 2390.
_sms_pal_probe:
        ld      hl,_frame_count
        ld      a,(hl)
probe_w:
        cp      (hl)            ; a frame begins
        jr      z,probe_w
        ld      a,(hl)
        ld      bc,0
probe_n:
        inc     bc              ; 6
        cp      (hl)            ; 7
        jr      z,probe_n       ; 12
        ld      hl,2620
        or      a
        sbc     hl,bc
        ld      l,0
        ret     nc
        inc     l
        ret

sms_frame:
        ld      a,(__tms9918_status_register)
        ld      hl,_vdp_seen
        or      (hl)
        ld      (hl),a
        ld      hl,(_frame_count)
        inc     hl
        ld      a,(_sms_pal)
        or      a
        jr      z,frame_n
        ld      a,(hz50_n)
        dec     a
        jr      nz,frame_5
        inc     hl              ; the fifth 50th: two 60ths
        ld      (_frame_count),hl
        call    _snd_frame
        ld      hl,(_frame_count)
        ld      a,5
frame_5:
        ld      (hz50_n),a
frame_n:
        ld      (_frame_count),hl
        ; (on into snd_frame)

; One frame of the sound effect: the tone's frames run out, then the
; second tone (if any) starts, then quiet. Changes only AF.
_snd_frame:
        ld      a,(_snd_f)
        or      a
        ret     z               ; nothing sounding (or snd_play at work)
        dec     a
        ld      (_snd_f),a
        ret     nz
        ld      a,(_snd_nf)
        or      a
        jr      z,snd_quiet
        ld      (_snd_f),a
        xor     a
        ld      (_snd_nf),a
        ld      a,(_snd_n)
        out     (PSG),a
        ld      a,(_snd_n+1)
        out     (PSG),a
        ld      a,(_snd_n+2)
        out     (PSG),a
        ret
snd_quiet:
        ld      a,$9F
        out     (PSG),a
        ret

        SECTION bss_user
_sms_pal:       defs    1
hz50_n:         defs    1
