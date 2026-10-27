;
; sprite.s -- gfx.h's sprites on the 7800: each shown slot is direct 160A
; objects in the zones it covers (maria.s), one 8-line piece a zone, made
; from the original's 1-bit shapes (the ART bank's atari_shapes) in pixel
; value 3: a palette's colour 3 in a 160A zone, colour 2 in a 320A one, so
; P4-P7 have the slot's colour in both (nmi.s). Four colours a frame; a
; fifth takes the last palette.
;
; A slot's pieces sit side by side in TXTB's columns COL_SPR on (a piece is
; its width in bytes: 2, 4 for a double-width shape, 8 for quad), and are
; made again only when the shape, frame, flip or line within the zone (y &
; 7) changes: moving across is the objects' x alone.
;
        .include "a78.inc"
        .export _spr_pieces
        .export _spr_on, _spr_shape, _spr_frame, _spr_xl, _spr_xh
        .export _spr_yl, _spr_yh, _spr_col, _spr_flags, _spr_changed
        .import _sp_n, _sp_lo, _sp_pw, _sp_x, _zflip, _spr_pal
        .import _atari_shapes, _bank_art_on, _bank_art_off
        .importzp ptr1, ptr2, tmp1, tmp2, tmp3, tmp4

SLOTS   = 10
SP_MAX  = 6
SPR_FLIPX = 1

        .bss
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
s_col:       .res SLOTS         ; its first column
s_ncol:      .res SLOTS         ; columns it has (0: none)
s_key:       .res SLOTS * 4     ; shape, frame, flags, y & 7 its pieces show
s_valid:     .res SLOTS         ; ... are made
old_n:       .res ROWS          ; each zone's pieces before this flush
slot:        .res 1
pw_w:           .res 1             ; a piece's width
sh_h:           .res 1             ; the shape's height
np:          .res 1             ; pieces
sh_d:           .res 1             ; y & 7
z0:          .res 1             ; its first zone (signed)
hp:          .res 1             ; its x (hpos)
pal:         .res 1             ; its palette << 5
need:        .res 1
cand:        .res 1
pk:           .res 1
ln:          .res 1
src:         .res 2             ; the frame's first byte
size:        .res 1
cols:        .res 4             ; the frame's colours in P4-P7
ncols:       .res 1

        .segment "REPLRO"       ; (in every bank: mule-a78.cfg)
dbl:    .byte   $00, $0F, $F0, $FF          ; 2 bits -> a double-width byte
exp3:                                       ; a nibble -> 4 pixels of value 3
        .repeat 16, N
        .byte (((N >> 3) & 1) << 6 | ((N >> 2) & 1) << 4 | ((N >> 1) & 1) << 2 | (N & 1)) * 3
        .endrep
rev:                                        ; a nibble backwards
        .byte   $0, $8, $4, $C, $2, $A, $6, $E, $1, $9, $5, $D, $3, $B, $7, $F
widths: .byte   2, 4, 8, 8                  ; size 0, 1, (2), 3

        .segment "REPL"

; void spr_pieces(void): every zone's pieces again, if a slot changed
; (gfx.c spr_flush: then maria.s builds the zones)
_spr_pieces:
        lda     _spr_changed
        bne     @go
        rts
@go:    lda     #0
        sta     _spr_changed
        sta     ncols
        ldx     #ROWS - 1
@keep:  lda     _sp_n,x
        sta     old_n,x
        lda     #0
        sta     _sp_n,x
        dex
        bpl     @keep
        jsr     _bank_art_on            ; (the shapes)
        lda     #0
        sta     slot
@slot:  ldx     slot
        lda     _spr_on,x
        bne     @on
        lda     #0                      ; off: its columns go back
        sta     s_ncol,x
        sta     s_valid,x
        jmp     @next
@on:    jsr     one
@next:  inc     slot
        lda     slot
        cmp     #SLOTS
        bcc     @slot
        jsr     _bank_art_off
        ; the colours, and the zones whose pieces changed
        ldx     #3
@pc:    lda     cols,x
        sta     _spr_pal,x
        dex
        bpl     @pc
        ldx     #ROWS - 1
@z:     lda     _sp_n,x
        ora     old_n,x
        beq     @nz
        lda     #1
        sta     _zflip,x
@nz:    dex
        bpl     @z
        rts

; slot X: its palette, columns, pieces, and its entries in the zones
one:    ; the palette: the colour's, or a new one
        lda     _spr_col,x
        ldy     #0
@f:     cpy     ncols
        bcs     @new
        cmp     cols,y
        beq     @have
        iny
        bne     @f
@new:   cpy     #4
        bcc     @add
        ldy     #3                      ; a fifth colour: the last palette
        bne     @have
@add:   sta     cols,y
        inc     ncols
@have:  tya
        clc
        adc     #4
        asl     a
        asl     a
        asl     a
        asl     a
        asl     a
        sta     pal
        ; the shape: data, height, size (atari_shapes[s]: ptr, sh_h, size)
        lda     _spr_shape,x
        asl     a
        asl     a
        tay
        lda     _atari_shapes,y
        sta     src
        lda     _atari_shapes+1,y
        sta     src+1
        lda     _atari_shapes+2,y
        sta     sh_h
        lda     _atari_shapes+3,y
        and     #3
        sta     size
        tay
        lda     widths,y
        sta     pw_w
        ; the frame: src += frame * sh_h
        lda     _spr_frame,x
        beq     @f0
        sta     tmp1
@fm:    lda     src
        clc
        adc     sh_h
        sta     src
        bcc     @fn
        inc     src+1
@fn:    dec     tmp1
        bne     @fm
@f0:    ; x: on screen from -32 to 159
        lda     _spr_xh,x
        beq     @xp
        cmp     #$FF
        bne     @h1
        lda     _spr_xl,x
        cmp     #$E0
        bcs     @xok
@h1:    rts
@xp:    lda     _spr_xl,x
        cmp     #160
        bcs     @h1
@xok:   sta     hp
        ; y: zone and line; from -32 to 191
        lda     _spr_yl,x
        and     #7
        sta     sh_d
        lda     _spr_yh,x
        sta     tmp2
        lda     _spr_yl,x
        lsr     tmp2
        ror     a
        lsr     tmp2
        ror     a
        lsr     tmp2
        ror     a               ; y >> 3 (arithmetic: tmp2 was 0 or $FF)
        ldy     _spr_yh,x
        beq     @yp
        ora     #$E0            ; negative: sign-extend
        cmp     #$FC
        bcs     @yok
@h2:    rts                     ; above -32
@yp:    cmp     #ROWS
        bcs     @h2
@yok:   sta     z0
        lda     sh_d               ; pieces: (sh_d + sh_h + 7) / 8
        clc
        adc     sh_h
        adc     #7
        lsr     a
        lsr     a
        lsr     a
        sta     np
        ; its columns: np * pw_w
        lda     #0
        ldy     np
@nc:    clc
        adc     pw_w
        dey
        bne     @nc
        sta     need
        cmp     s_ncol,x
        beq     @cols
        bcc     @cols
        jsr     alloc
        bcs     @hide
@cols:  ; made already?
        txa
        asl     a
        asl     a
        tay
        lda     _spr_shape,x
        cmp     s_key,y
        bne     @make
        lda     _spr_frame,x
        cmp     s_key+1,y
        bne     @make
        lda     _spr_flags,x
        cmp     s_key+2,y
        bne     @make
        lda     sh_d
        cmp     s_key+3,y
        bne     @make
        lda     s_valid,x
        bne     @list
@make:  lda     _spr_shape,x
        sta     s_key,y
        lda     _spr_frame,x
        sta     s_key+1,y
        lda     _spr_flags,x
        sta     s_key+2,y
        lda     sh_d
        sta     s_key+3,y
        lda     #1
        sta     s_valid,x
        jsr     make
        ldx     slot
@list:  jmp     list
@hide:  rts

; slot X needs `need` columns: the first gap that holds them (carry set:
; none). Its old ones go back first.
alloc:  lda     #0
        sta     s_ncol,x
        sta     s_valid,x
        lda     #COL_SPR
        sta     cand
@try:   lda     cand            ; past the end?
        clc
        adc     need
        bcs     @none
        ldy     #SLOTS - 1
@o:     lda     s_ncol,y        ; overlaps slot Y's?
        beq     @no
        lda     cand
        clc
        adc     need
        cmp     s_col,y         ; cand + need <= its start: clear
        beq     @no
        bcc     @no
        lda     s_col,y
        clc
        adc     s_ncol,y
        bcs     @ov             ; (its end past 255: it overlaps)
        cmp     cand            ; its end <= cand: clear
        beq     @no
        bcc     @no
@ov:    lda     s_col,y         ; overlaps: try after it
        clc
        adc     s_ncol,y
        bcs     @none
        sta     cand
        jmp     @try
@no:    dey
        bpl     @o
        lda     cand
        sta     s_col,x
        lda     need
        sta     s_ncol,x
        clc
        rts
@none:  sec
        rts

; slot X's pieces: piece pk, line ln = shape row 8k + ln - sh_d (blank outside)
make:   lda     #0
        sta     pk
@piece: lda     #0
        sta     ln
        ; the piece's first column: s_col + pk * pw_w
        ldx     slot
        lda     s_col,x
        ldy     pk
        beq     @c0
@cw:    clc
        adc     pw_w
        dey
        bne     @cw
@c0:    sta     ptr2            ; (lo: the column)
@line:  lda     #>TXTB + 7
        sec
        sbc     ln
        sta     ptr2+1
        ; the row: 8k + ln - sh_d
        lda     pk
        asl     a
        asl     a
        asl     a
        clc
        adc     ln
        sec
        sbc     sh_d
        bcc     @blank
        cmp     sh_h
        bcs     @blank
        tay
        lda     src
        sta     ptr1
        lda     src+1
        sta     ptr1+1
        lda     (ptr1),y
        ldx     slot
        ldy     _spr_flags,x
        beq     @put
        jsr     flip
        jmp     @put
@blank: lda     #0
@put:   jsr     expand
        inc     ln
        lda     ln
        cmp     #8
        bcc     @line
        inc     pk
        lda     pk
        cmp     np
        bcc     @piece
        rts

; A backwards (SPR_FLIPX)
flip:   sta     tmp1
        and     #$0F
        tax
        lda     rev,x
        asl     a
        asl     a
        asl     a
        asl     a
        sta     tmp2
        lda     tmp1
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        tax
        lda     rev,x
        ora     tmp2
        rts

; A's 8 pixels in value 3 at (ptr2): pw_w bytes (size 0: each pixel a 160A
; pixel; 1: two; 3: four)
expand: sta     tmp1
        ldx     size
        beq     @s0
        cpx     #1
        beq     @s1
        ; quad: a pixel a byte
        ldy     #0
@q:     lda     #0
        asl     tmp1
        bcc     @q0
        lda     #$FF
@q0:    sta     (ptr2),y
        iny
        cpy     #8
        bcc     @q
        rts
@s1:    ldy     #0              ; double: two pixels a byte
@d:     lda     tmp1
        rol     a
        rol     a
        rol     a
        and     #3
        tax
        lda     dbl,x
        sta     (ptr2),y
        asl     tmp1
        asl     tmp1
        iny
        cpy     #4
        bcc     @d
        rts
@s0:    lda     tmp1
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        tax
        lda     exp3,x
        ldy     #0
        sta     (ptr2),y
        lda     tmp1
        and     #$0F
        tax
        lda     exp3,x
        iny
        sta     (ptr2),y
        rts

; slot X's pieces into the zones they show in
list:   lda     #0
        sta     pk
@p:     lda     z0
        clc
        adc     pk
        cmp     #ROWS           ; (negative ones are >= 24 too)
        bcs     @n
        tay
        lda     _sp_n,y
        cmp     #SP_MAX
        bcs     @n
        sta     tmp1
        tya                     ; zone * SP_MAX + n
        asl     a
        sta     tmp2
        asl     a
        adc     tmp2
        adc     tmp1
        tax
        tya
        pha
        ldy     slot
        lda     s_col,y
        ldy     pk
        beq     @c
@cw:    clc
        adc     pw_w
        dey
        bne     @cw
@c:     sta     _sp_lo,x
        lda     #0
        sec
        sbc     pw_w
        and     #$1F
        ora     pal
        sta     _sp_pw,x
        lda     hp
        sta     _sp_x,x
        pla
        tay
        lda     _sp_n,y
        clc
        adc     #1
        sta     _sp_n,y
@n:     inc     pk
        lda     pk
        cmp     np
        bcc     @p
        rts
