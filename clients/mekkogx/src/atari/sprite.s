;
; sprite.s -- gfx.c's sprites on the players, and the row flags.
;
; spr_flush puts each shown sprite (in the order of order:) on the first
; player that is free in the rows (of 8 scanlines) it covers; the DLI moves and colours the players row
; by row (gfxa.s). A player remembers what it drew (up to 4 pieces) so the
; next flush can clear just that.
;
; The flush only places the pieces: their bytes go into the players in the
; deferred VBI that takes the rows (pmg_apply), so a piece never shows a
; frame at the place its player had before (a figure's new frame where it
; stood, a dash of the ship in another sprite's row).
;
        .export _spr_flush, _row_flags_update, _spr_players, _spr_forget
        .export _bm_ready, pmg_apply
        .export _spr_on, _spr_shape, _spr_frame, _spr_xl, _spr_xh
        .export _spr_yl, _spr_yh, _spr_col, _spr_flags, _spr_changed, _sp_dirty, _pf_dirty
        .import _row_flags, _row_chb, _row_pf, _row_hp, _row_cm, _row_sz, _dsp_ready
        .import _atari_shapes, _row_mode, _row_used, _pfuse
        .importzp ptr1, ptr2, tmp1, tmp2, tmp3, tmp4

SLOTS   = 10
ROWS    = 24
MAXSEG  = 4
        .include "hw.inc"
SPR_FLIPX = 1

        .segment "BSS2"         ; low memory (mule-atari.cfg), cleared by clrlow
_spr_on:     .res SLOTS
_spr_shape:  .res SLOTS
_spr_frame:  .res SLOTS
_spr_xl:     .res SLOTS
_spr_xh:     .res SLOTS
_spr_yl:     .res SLOTS
_spr_yh:     .res SLOTS
_spr_col:    .res SLOTS         ; the colour register value
_spr_flags:  .res SLOTS
_spr_changed: .res 1
_sp_dirty:   .res 1
_pf_dirty:   .res 1
_spr_players: .res 1            ; how many players the sprites may use
seg_n:       .res 4
seg_y:       .res 4 * MAXSEG
seg_h:       .res 4 * MAXSEG
old_n:       .res 4             ; the pieces shown before this flush
old_y:       .res 4 * MAXSEG
old_h:       .res 4 * MAXSEG
seg_sl:      .res 4 * MAXSEG    ; a piece's bytes (lo, hi), mirrored or not
seg_sh:      .res 4 * MAXSEG
seg_fl:      .res 4 * MAXSEG
_bm_ready:   .res 1             ; pieces placed, not yet drawn (pmg_apply)
v_pl:        .res 1             ; pmg_apply's own: it interrupts anything
v_i:         .res 1
v_n:         .res 1
v_h:         .res 1
v_fl:        .res 1
v_b:         .res 1
v_t:         .res 1
v_y:         .res 1
v_e:         .res 1
own:         .res 4 * ROWS
slot:        .res 1
ord:         .res 1             ; place in order
sh:          .res 1             ; height (clipped)
ssize:       .res 1
sy:          .res 1             ; first scanline
r0:          .res 1
r1:          .res 1
pl:          .res 1
hpos:        .res 1
rowoff:      .res 4             ; p * ROWS

; Clear the low variables ($0400-$09FF: LOWBSS, BSS2 in mule-atari.cfg)
; before main; zerobss only clears BSS. (The 5200's crt0 clears its RAM.)
.ifndef __ATARI5200__
        .constructor clrlow
.endif

        .segment "ZEROPAGE"
vsrc:   .res 2                  ; pmg_apply's
vdst:   .res 2

        .rodata
p_rows: .byte 0, ROWS, ROWS*2, ROWS*3
rev:    .byte   $0, $8, $4, $C, $2, $A, $6, $E, $1, $9, $5, $D, $3, $B, $7, $F
revhi:  .repeat 16, N           ; a nibble backwards, in the high one
        .byte   ((N & 1) << 7) | ((N & 2) << 5) | ((N & 4) << 3) | ((N & 8) << 1)
        .endrep
; the order slots get players in: the ship's nose (5) last, so on the map
; (two players) the hull's two halves get them rather than nose and top
order:  .byte 0, 1, 2, 3, 4, 6, 7, 8, 9, 5


.ifndef __ATARI5200__
        .segment "ONCE"
clrlow: lda     #0
        tax
@c:     sta     $0400,x
        sta     $0500,x
        sta     $0600,x
        sta     $0700,x
        sta     $0800,x
        sta     $0900,x
        inx
        bne     @c
        rts
.endif

        .code

; void spr_flush(void)
_spr_flush:
        lda     _spr_changed
        bne     @go
        jmp     flags
@go:    lda     #0
        sta     _spr_changed
        sta     _dsp_ready      ; (the VBI takes none of a half-built set)
        lda     #1
        sta     _sp_dirty
        ; the players' rows afresh
        ldx     #0
@clr:   cpx     _spr_players
        bcs     @cleared
        stx     pl
        lda     #0
        sta     seg_n,x
        ldy     p_rows,x
        ldx     #ROWS
@rows:  sta     own,y
        sta     _row_hp,y
        sta     _row_sz,y
        iny
        dex
        bne     @rows
        ldx     pl
        inx
        bne     @clr
@cleared:
        ldx     #0
@slot:  stx     ord
        lda     order,x
        tax
        stx     slot
        lda     _spr_on,x
        bne     @on
@next:  ldx     ord
        inx
        cpx     #SLOTS
        bcc     @slot
        lda     #1              ; the pieces, for pmg_apply
        sta     _bm_ready
        jmp     smooth
@on:    ; the shape: data, h, size
        lda     _spr_shape,x
        asl     a
        asl     a
        tay
        lda     _atari_shapes,y
        sta     ptr2
        lda     _atari_shapes+1,y
        sta     ptr2+1
        lda     _atari_shapes+2,y
        sta     sh
        lda     _atari_shapes+3,y
        sta     ssize
        ; x: shown if -32 <= x < 168 -> hpos = x + 48
        lda     _spr_xl,x
        clc
        adc     #48
        sta     hpos
        lda     _spr_xh,x
        adc     #0
        bne     @next           ; off either side
        ; src = data + frame * h
        lda     _spr_frame,x
        sta     tmp1
        beq     @nof
@mul:   lda     ptr2
        clc
        adc     sh
        sta     ptr2
        bcc     @m2
        inc     ptr2+1
@m2:    dec     tmp1
        bne     @mul
@nof:   ; y: clip to 0..191
        ldx     slot
        lda     _spr_yh,x
        bmi     @neg
        bne     @next           ; >= 256
        lda     _spr_yl,x
        cmp     #192
        bcs     @next
        sta     sy
        jmp     @bot
@neg:   cmp     #$FF
        bne     @next
        lda     _spr_yl,x       ; -1..-255: skip -y bytes of the shape
        eor     #$FF
        clc
        adc     #1              ; -y
        cmp     sh
        bcs     @next           ; all above the top
        sta     tmp1
        lda     sh
        sec
        sbc     tmp1
        sta     sh
        lda     ptr2
        clc
        adc     tmp1
        sta     ptr2
        bcc     @n2
        inc     ptr2+1
@n2:    lda     #0
        sta     sy
@bot:   lda     sy              ; h = min(h, 192 - y)
        clc
        adc     sh
        bcs     @cut
        cmp     #193
        bcc     @hok
@cut:   lda     #192
        sec
        sbc     sy
        sta     sh
@hok:   lda     sh
        bne     @h1
        jmp     @next
@h1:    lda     sy
        lsr     a
        lsr     a
        lsr     a
        sta     r0
        lda     sy
        clc
        adc     sh
        sec
        sbc     #1
        lsr     a
        lsr     a
        lsr     a
        sta     r1
        ; a free player
        ldx     #0
@try:   cpx     _spr_players
        bcc     @t1
        jmp     @next           ; none: not shown
@t1:    lda     seg_n,x
        cmp     #MAXSEG
        bcs     @busy
        lda     p_rows,x
        clc
        adc     r0
        tay
        lda     r0
        sta     tmp1
@tr:    lda     own,y
        bne     @busy
        iny
        lda     tmp1
        cmp     r1
        inc     tmp1
        bcc     @tr
        jmp     @got
@busy:  inx
        bne     @try
@got:   stx     pl
        ; remember the piece
        txa
        asl     a
        asl     a
        clc
        adc     seg_n,x
        tay
        inc     seg_n,x
        lda     sy
        clc
        adc     #32             ; scanline -> PMG byte
        sta     seg_y,y
        lda     sh
        sta     seg_h,y
        lda     ptr2            ; its bytes, mirrored if asked
        sta     seg_sl,y
        lda     ptr2+1
        sta     seg_sh,y
        ldx     slot
        lda     _spr_flags,x
        and     #SPR_FLIPX
        sta     seg_fl,y
        ; its rows: position, colour, size
        ldx     pl
        lda     p_rows,x
        clc
        adc     r0
        tay
        ldx     slot
        lda     _spr_col,x
        sta     tmp3
        lda     r0
        sta     tmp1
@sr:    lda     #1
        sta     own,y
        lda     hpos
        sta     _row_hp,y
        lda     tmp3
        sta     _row_cm,y
        lda     ssize
        sta     _row_sz,y
        iny
        lda     tmp1
        cmp     r1
        inc     tmp1
        bcc     @sr
        jmp     @next

; A player's rows without a piece take the next piece's position, size and
; colour (its bytes there are 0: nothing shows). The DLI's writes land a
; little into a row's first scanline, so a shape starting on that line was
; drawn there with the row above's position and size (a stray dash above
; the ship's hull); now that row changes nothing.
smooth: ldx     #0
@pl:    cpx     _spr_players
        bcs     flags
        stx     pl
        lda     p_rows,x
        clc
        adc     #ROWS - 2
        tay
        lda     #ROWS - 1
        sta     tmp1
@r:     lda     own,y
        bne     @u
        lda     _row_hp+1,y
        sta     _row_hp,y
        lda     _row_sz+1,y
        sta     _row_sz,y
        lda     _row_cm+1,y
        sta     _row_cm,y
@u:     dey
        dec     tmp1
        bne     @r
        ldx     pl
        inx
        bne     @pl

flags:  lda     _sp_dirty
        ora     _pf_dirty
        beq     @out
        lda     #0
        sta     _sp_dirty
        sta     _pf_dirty
        jmp     _row_flags_update
@out:   rts

; void spr_forget(void): the players are blank (a new screen): nothing drawn
; to clear, nothing placed to draw
_spr_forget:
        lda     #0
        sta     _dsp_ready
        sta     _bm_ready
        ldx     #3
@f:     sta     seg_n,x
        sta     old_n,x
        dex
        bpl     @f
        rts

; pmg_apply (gfxa.s vbid, before the rows it goes with): each player's new
; pieces drawn, then the bytes of the pieces it showed before that none of
; them covers cleared (a piece drawn again in place never blanks). What it
; showed is then what was placed.
pmg_apply:
        lda     #0
        sta     _bm_ready
        tax
@pl:    cpx     _spr_players
        bcc     @do
        jmp     @kept
@do:    stx     v_pl
        txa
        clc
        adc     #>PLAYERS
        sta     vdst+1
        txa
        asl     a
        asl     a
        sta     v_i             ; p * MAXSEG
        lda     seg_n,x
        sta     v_n
@seg:   lda     v_n
        beq     @old
        dec     v_n
        ldx     v_i
        inc     v_i
        lda     seg_y,x
        sta     vdst
        lda     seg_h,x
        sta     v_h
        lda     seg_sl,x
        sta     vsrc
        lda     seg_sh,x
        sta     vsrc+1
        lda     seg_fl,x
        sta     v_fl
        ldy     #0
@cp:    lda     (vsrc),y
        ldx     v_fl
        beq     @put
        sta     v_b             ; mirrored: each nibble backwards, swapped
        and     #$0F
        tax
        lda     revhi,x
        sta     v_t
        lda     v_b
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        tax
        lda     rev,x
        ora     v_t
@put:   sta     (vdst),y
        iny
        cpy     v_h
        bcc     @cp
        bcs     @seg
@old:   lda     #0
        sta     vdst
        ldx     v_pl
        txa
        asl     a
        asl     a
        sta     v_i
        lda     old_n,x
        sta     v_n
@os:    lda     v_n
        beq     @np
        dec     v_n
        ldx     v_i
        inc     v_i
        lda     old_y,x
        sta     v_y
        clc
        adc     old_h,x
        sta     v_e
@by:    ldx     v_pl            ; is v_y in a new piece of this player?
        ldy     seg_n,x
        txa
        asl     a
        asl     a
        tax
@ck:    dey
        bmi     @zero
        lda     v_y
        cmp     seg_y,x
        bcc     @cn
        sbc     seg_y,x
        cmp     seg_h,x
        bcc     @in
@cn:    inx
        bne     @ck
@zero:  ldy     v_y
        lda     #0
        sta     (vdst),y
@in:    inc     v_y
        lda     v_y
        cmp     v_e
        bcc     @by
        bcs     @os
@np:    ldx     v_pl
        inx
        jmp     @pl
@kept:  ldx     #4 + 8 * MAXSEG - 1
@k:     lda     seg_n,x
        sta     old_n,x
        dex
        bpl     @k
        rts

; void row_flags_update(void): what each row's DLI loads (what differs from
; the row above; row 0 everything). First the colour registers a row doesn't
; use take the next row's values, so fewer change between rows: a change
; lands a little into the row's first scanline.
_row_flags_update:
        lda     #0
        sta     _dsp_ready
        ldx     #ROWS - 2
@sm:    ldy     _row_mode,x
        lda     _pfuse,y
        bpl     @m
        lda     _row_used,x     ; big and drawing rows: the slots taken
@m:     sta     tmp2
        lsr     tmp2
        bcs     @1
        lda     _row_pf+1,x
        sta     _row_pf,x
@1:     lsr     tmp2
        bcs     @2
        lda     _row_pf+ROWS+1,x
        sta     _row_pf+ROWS,x
@2:     lsr     tmp2
        bcs     @3
        lda     _row_pf+ROWS*2+1,x
        sta     _row_pf+ROWS*2,x
@3:     lsr     tmp2
        bcs     @4
        lda     _row_pf+ROWS*3+1,x
        sta     _row_pf+ROWS*3,x
@4:     dex
        bpl     @sm
        lda     #$0F
        sta     _row_flags
        ldx     #1
@row:   lda     #0
        sta     tmp1
        lda     _row_chb,x
        cmp     _row_chb-1,x
        bne     @pf
        lda     _row_pf,x
        cmp     _row_pf-1,x
        bne     @pf
        lda     _row_pf+ROWS,x
        cmp     _row_pf+ROWS-1,x
        bne     @pf
        lda     _row_pf+ROWS*2,x
        cmp     _row_pf+ROWS*2-1,x
        bne     @pf
        lda     _row_pf+ROWS*3,x
        cmp     _row_pf+ROWS*3-1,x
        beq     @hp
@pf:    lda     #1
        sta     tmp1
@hp:    lda     _row_hp,x
        cmp     _row_hp-1,x
        bne     @hp1
        lda     _row_hp+ROWS,x
        cmp     _row_hp+ROWS-1,x
        bne     @hp1
        lda     _row_hp+ROWS*2,x
        cmp     _row_hp+ROWS*2-1,x
        bne     @hp1
        lda     _row_hp+ROWS*3,x
        cmp     _row_hp+ROWS*3-1,x
        beq     @cm
@hp1:   lda     tmp1
        ora     #2
        sta     tmp1
@cm:    lda     _row_cm,x
        cmp     _row_cm-1,x
        bne     @cm1
        lda     _row_cm+ROWS,x
        cmp     _row_cm+ROWS-1,x
        bne     @cm1
        lda     _row_cm+ROWS*2,x
        cmp     _row_cm+ROWS*2-1,x
        bne     @cm1
        lda     _row_cm+ROWS*3,x
        cmp     _row_cm+ROWS*3-1,x
        beq     @sz
@cm1:   lda     tmp1
        ora     #4
        sta     tmp1
@sz:    lda     _row_sz,x
        cmp     _row_sz-1,x
        bne     @sz1
        lda     _row_sz+ROWS,x
        cmp     _row_sz+ROWS-1,x
        bne     @sz1
        lda     _row_sz+ROWS*2,x
        cmp     _row_sz+ROWS*2-1,x
        bne     @sz1
        lda     _row_sz+ROWS*3,x
        cmp     _row_sz+ROWS*3-1,x
        beq     @st
@sz1:   lda     tmp1
        ora     #8
        sta     tmp1
@st:    lda     tmp1
        sta     _row_flags,x
        inx
        cpx     #ROWS
        bcs     @done
        jmp     @row
@done:  lda     #1              ; a whole set: the deferred VBI takes it
        sta     _dsp_ready
        rts
