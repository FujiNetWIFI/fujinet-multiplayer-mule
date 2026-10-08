; sprout.asm -- a VDP sprite's pattern sent in its colour (vram_sms.h's
; spr_pattern): the SMS keeps a sprite's colour in its tiles, so each of
; the 32 bytes goes out four times, once a bit plane, all or nothing as
; the colour has that bit. The writes are 26 cycles apart or more (what
; the VDP wants while it draws the screen).

        SECTION code_user

        PUBLIC  _spr_out
        EXTERN  _so_addr, _so_pat, _so_col

        defc    VDP_DATA = $BE
        defc    VDP_CTRL = $BF

; void spr_out(void): so_pat's 32 bytes at VRAM so_addr, colour so_col.
_spr_out:
        push    ix
        ld      a,(_so_col)
        ld      c,a
        rr      c               ; the planes' masks: D E IXH IXL
        sbc     a,a
        ld      d,a
        rr      c
        sbc     a,a
        ld      e,a
        rr      c
        sbc     a,a
        ld      ixh,a
        rr      c
        sbc     a,a
        ld      ixl,a
        ld      hl,(_so_addr)
        di                      ; (the interrupt reads the status: the
        ld      a,l             ; address latch)
        out     (VDP_CTRL),a
        ld      a,h
        or      $40
        out     (VDP_CTRL),a
        ei
        ld      hl,(_so_pat)
        ld      b,32
so_byte:
        ld      a,(hl)          ; 7
        and     d               ; 4
        out     (VDP_DATA),a    ; 11
        ld      a,(hl)          ; 7
        and     e               ; 4
        nop                     ; 4
        out     (VDP_DATA),a    ; 11: 26
        ld      a,(hl)          ; 7
        and     ixh             ; 8
        out     (VDP_DATA),a    ; 11: 26
        ld      a,(hl)          ; 7
        and     ixl             ; 8
        out     (VDP_DATA),a    ; 11: 26
        inc     hl
        djnz    so_byte
        pop     ix
        ret
