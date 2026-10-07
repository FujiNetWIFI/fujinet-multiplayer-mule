;
; sprite.s -- gfx.c's sprites on the players, and the row flags.
;
; spr_flush puts each shown sprite (in the order of order:) on the first
; player that is free in the rows (of 8 scanlines) it covers; the DLI moves and colours the players row
; by row (gfxa.s). A player remembers what it drew (up to 4 pieces) so the
; next flush can clear just that.
;
        .export _spr_flush, _row_flags_update, _spr_players
        .export _spr_on, _spr_shape, _spr_frame, _spr_xl, _spr_xh
        .export _spr_yl, _spr_yh, _spr_col, _spr_flags, _spr_changed, _sp_dirty, _pf_dirty
        .import _row_flags, _row_chb, _row_pf, _row_hp, _row_cm, _row_sz
        .import _atari_shapes, _row_mode, _row_used, _pfuse
        .importzp ptr1, ptr2, tmp1, tmp2, tmp3, tmp4

SLOTS   = 10
ROWS    = 24
MAXSEG  = 4
PLAYERS = $BC00
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
; before main; zerobss only clears BSS.
        .constructor clrlow

        .rodata
p_rows: .byte 0, ROWS, ROWS*2, ROWS*3
; the order slots get players in: the ship's nose (5) last, so on the map
; (two players) the hull's two halves get them rather than nose and top
order:  .byte 0, 1, 2, 3, 4, 6, 7, 8, 9, 5


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

        .code

; void spr_flush(void)
_spr_flush:
        lda     _spr_changed
        bne     @go
        jmp     flags
@go:    lda     #0
        sta     _spr_changed
        lda     #1
        sta     _sp_dirty
        ; clear what the players showed
        ldx     #0
@clr:   cpx     _spr_players
        bcs     @cleared
        stx     pl
        txa
        clc
        adc     #>PLAYERS
        sta     ptr1+1
        lda     seg_n,x
        sta     tmp1            ; pieces
        txa
        asl     a
        asl     a
        sta     tmp2            ; p * MAXSEG
@seg:   lda     tmp1
        beq     @segdone
        dec     tmp1
        ldx     tmp2
        lda     seg_y,x
        sta     ptr1
        ldy     seg_h,x
        inc     tmp2
        lda     #0
@z:     dey
        sta     (ptr1),y
        bne     @z
        beq     @seg
@segdone:
        ldx     pl
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
        sta     ptr1
        lda     sh
        sta     seg_h,y
        txa
        clc
        adc     #>PLAYERS
        sta     ptr1+1
        ; copy (mirrored if asked)
        ldx     slot
        lda     _spr_flags,x
        and     #SPR_FLIPX
        sta     tmp3
        ldy     #0
@cp:    lda     (ptr2),y
        ldx     tmp3
        beq     @put
        sta     tmp4            ; mirror the byte
        lda     #0
        ldx     #8
@fl:    lsr     tmp4
        rol     a
        dex
        bne     @fl
@put:   sta     (ptr1),y
        iny
        cpy     sh
        bcc     @cp
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

; void row_flags_update(void): what each row's DLI loads (what differs from
; the row above; row 0 everything). First the colour registers a row doesn't
; use take the next row's values, so fewer change between rows: a change
; lands a little into the row's first scanline.
_row_flags_update:
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
@done:  rts
