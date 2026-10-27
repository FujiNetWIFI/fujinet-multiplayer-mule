;
; mapgfx.s -- gfx.c's map on the 7800: the plots in the original's map
; characters (the Atari port's src/atari/mapgfx.s), and what the original
; drew with players as terrain objects in each zone's display list
; (maria.s): the mountains one to a row, the river, the store; and the time
; bar, a piece a row. Their pictures are pieces in TXTB's spare columns
; (a78.inc COL_*), made by gfx.c; their colours are colour 1 of P4-P7.
;
; A map row's characters take their colour from bits 6-7 (PF0-3); each plot
; row (four display rows) has its own four colours, counted by the plots
; that hold them, so a fifth owner colour (white, for a plot on offer) can
; take a slot nobody in that row is using.
;
; Also MTB's glyphs: a map glyph in its three values (map_glyphs), the
; store's fire into two of them (map_glyph), the town's characters.
;
        .include "a78.inc"
        .export _gfx_plot, _map_slot, _map_row_pf, _map_reset, _map_bar
        .export _mp_uses, _mbar, _mbar_col, _map_pr, _tbar_x, _map_glyph
        .export _mtb_glyph, _mtb_town, _expand_v
        .import _row_pf, _row_lo, _row_hi, _atari_col, _pf_dirty, _zdirty
        .import _tr_lo, _tr_pw, _tr_x, _ter_pal, _mapidx
        .importzp gp
        .import t_code
        .import incsp1, incsp2
        .importzp c_sp, ptr1, ptr2, tmp1, tmp2, tmp3, tmp4

ROW0    = 2                     ; the map's first display row
C_BLUE  = 2
TR_MTN  = 0                     ; a zone's terrain objects (maria.s: 4)
TR_RIV  = 1
TR_STORE = 2
TR_BAR  = 3

        .bss
mp_col:   .res 20               ; plot row * 4 + slot: the colour
_mp_uses: .res 20               ; ... and how many plots hold it
plot_slot: .res 45
plot_mtn: .res 45
_mbar:    .res ROWS             ; the time bar: lines lit, bit 7 the top
_mbar_col: .res 1
_tbar_x:   .res 1
_map_pr:
pr:       .res 1                ; the plot's row, column
pc:       .res 1
pp:       .res 1
lc:       .res 4                ; its left and right characters
rc:       .res 4
band:     .res 1
cnt:      .res 1
kk:       .res 1
vv:       .res 1

        .rodata
pick:   .byte   2, 0, 0,  1, 3, 0,  0, 2, 1     ; mountain shapes for 1, 2, 3
dx:     .byte   3, 7, 1, 5
pfoff:  .byte   0, ROWS, ROWS*2, ROWS*3
border_l: .byte $1D, $1E, $1E, $1F
border_r: .byte $20, $21, $21, $22
; a nibble of 1bpp pixels as one 160A byte in value 1, 2, 3
_expand_v:
        .repeat 16, N
        .byte ((N >> 3) & 1) << 6 | ((N >> 2) & 1) << 4 | ((N >> 1) & 1) << 2 | (N & 1)
        .endrep
        .repeat 16, N
        .byte (((N >> 3) & 1) << 6 | ((N >> 2) & 1) << 4 | ((N >> 1) & 1) << 2 | (N & 1)) * 2
        .endrep
        .repeat 16, N
        .byte (((N >> 3) & 1) << 6 | ((N >> 2) & 1) << 4 | ((N >> 1) & 1) << 2 | (N & 1)) * 3
        .endrep

        .code

; void map_reset(void): the plot rows' colours are the players' again, no
; terrain, no bar
_map_reset:
        ldx     #19
@c:     txa
        and     #3
        tay
        lda     _atari_col+C_BLUE,y
        sta     mp_col,x
        lda     #0
        sta     _mp_uses,x
        dex
        bpl     @c
        ldx     #44
@p:     lda     #0
        sta     plot_mtn,x
        lda     #$FF
        sta     plot_slot,x
        dex
        bpl     @p
        lda     #0
        ldx     #ROWS - 1
@b:     sta     _mbar,x
        dex
        bpl     @b
        ldx     #ROWS * 4 - 1
@t:     sta     _tr_pw,x
        dex
        bpl     @t
        ldx     #4
@r:     txa
        pha
        jsr     _map_row_pf
        pla
        tax
        dex
        bpl     @r
        rts

; void __fastcall__ map_row_pf(uint8_t pr): its colours into its 4 rows
_map_row_pf:
        sta     tmp1
        asl     a
        asl     a
        sta     tmp2            ; pr * 4: its colours, its first row - 2
        clc
        adc     #ROW0
        sta     tmp3            ; its first display row
        ldx     #3
@slot:  txa
        clc
        adc     tmp2
        tay
        lda     mp_col,y
        sta     tmp4
        lda     pfoff,x
        clc
        adc     tmp3
        tay
        lda     tmp4
        sta     _row_pf,y
        sta     _row_pf+1,y
        sta     _row_pf+2,y
        sta     _row_pf+3,y
        dex
        bpl     @slot
        lda     #1
        sta     _pf_dirty
        rts

; uint8_t __fastcall__ map_slot(uint8_t col): a colour slot in plot row pr
; (held: _mp_uses counts it)
_map_slot:
        tay
        lda     _atari_col,y
        sta     tmp1
        lda     pr
        asl     a
        asl     a
        sta     tmp2
        tay
        ldx     #0
@find:  lda     mp_col,y
        cmp     tmp1
        beq     @got
        iny
        inx
        cpx     #4
        bcc     @find
        ldy     tmp2
        ldx     #0
@free:  lda     _mp_uses,y
        beq     @take
        iny
        inx
        cpx     #4
        bcc     @free
        dey                     ; full: the last slot
        ldx     #3
        bne     @got
@take:  lda     tmp1
        sta     mp_col,y
        tya
        pha
        txa
        pha
        lda     pr
        jsr     _map_row_pf
        pla
        tax
        pla
        tay
@got:   lda     _mp_uses,y
        clc
        adc     #1
        sta     _mp_uses,y
        txa
        ldx     #0
        rts

; A = display row: the address of its first character in ptr1
rowaddr:
        tax
        lda     _row_lo,x
        sta     ptr1
        lda     _row_hi,x
        sta     ptr1+1
        rts

; Y = the zone, A = terrain kind: X = its index (zone * 4 + kind)
tridx:  sta     tmp4
        tya
        asl     a
        asl     a
        ora     tmp4
        tax
        rts

; void __fastcall__ map_bar(uint8_t row): the time bar's piece in that row,
; 4 colour clocks wide at bar_x (lines lit: _mbar, bit 7 the top), in
; colour 1 of P7
_map_bar:
        sta     tmp3
        tax
        lda     _mbar_col
        tay
        lda     _atari_col,y
        sta     _ter_pal+3
        ; the piece: line k at TXTB + (7 - k) * 256 + COL_BAR + row
        lda     #<(TXTB + COL_BAR)
        clc
        adc     tmp3
        sta     ptr2
        lda     #>TXTB + 7
        sta     ptr2+1
        lda     _mbar,x
        sta     tmp1
        ldy     #0
        ldx     #8
@l:     lda     #0
        asl     tmp1
        bcc     @o
        lda     #$55
@o:     sta     (ptr2),y
        dec     ptr2+1
        dex
        bne     @l
        ; there or not
        ldy     tmp3
        lda     #TR_BAR
        jsr     tridx
        ldy     tmp3
        lda     _mbar,y
        beq     @off
        lda     #<(COL_BAR)
        clc
        adc     tmp3
        sta     _tr_lo,x
        lda     _tbar_x
        sta     _tr_x,x
        lda     _tr_pw,x
        bne     @r              ; it was there: the pixels are enough
        lda     #(7 << 5) | ((-1) & $1F)        ; P7, 1 byte
        bne     @set
@off:   lda     _tr_pw,x
        beq     @r
        lda     #0
@set:   sta     _tr_pw,x
        lda     #1
        sta     _zdirty,y
@r:     rts

; void __fastcall__ gfx_plot(uint8_t p, const PlotLook *look)
; PlotLook: store, river, mountains, owned, icon, dots, col
_gfx_plot:
        sta     ptr2
        stx     ptr2+1
        ldy     #0
        lda     (c_sp),y
        sta     pp
        ; row and column
        ldx     #0
@div:   cmp     #9
        bcc     @dv
        sbc     #9
        inx
        bne     @div
@dv:    sta     pc
        stx     pr
        ; let go of its colour
        ldx     pp
        lda     plot_slot,x
        bmi     @nos
        sta     tmp1
        lda     pr
        asl     a
        asl     a
        clc
        adc     tmp1
        tay
        lda     _mp_uses,y
        sec
        sbc     #1
        sta     _mp_uses,y
        lda     #$FF
        sta     plot_slot,x
@nos:   lda     #0
        ldx     #3
@z:     sta     lc,x
        sta     rc,x
        dex
        bpl     @z
        ldy     #3              ; owned?
        lda     (ptr2),y
        beq     @jch
        ldy     #0              ; ... and not the store
        lda     (ptr2),y
        beq     @own
@jch:   jmp     @chars
@own:
        ldy     #6
        lda     (ptr2),y
        jsr     _map_slot
        ldx     pp
        sta     plot_slot,x
        lsr     a
        ror     a
        ror     a
        sta     kk              ; slot << 6
        ldx     #3
@b:     lda     border_l,x
        sta     lc,x
        lda     border_r,x
        sta     rc,x
        dex
        bpl     @b
        ldy     #4              ; icon: two characters, $11+ and $17+
        lda     (ptr2),y
        beq     @dots
        clc
        adc     #$10
        sta     rc+1
        adc     #6
        sta     rc+2
@dots:  ldy     #5              ; production: dots in the corners, $23-$2C
        lda     (ptr2),y
        beq     @col
        cmp     #5
        bcc     @d4
        pha
        lda     #$27
        sta     lc+3
        pla
        cmp     #6
        bcc     @d5
        inc     lc+3
@d5:    cmp     #7
        bcc     @d4b
        pha
        lda     #$2B
        sta     lc+2
        pla
        cmp     #8
        bcc     @d4b
        inc     lc+2
@d4b:   lda     #4
@d4:    clc
        adc     #$22
        sta     rc+3
@col:   ldx     #3
@or:    lda     lc,x
        ora     kk
        sta     lc,x
        lda     rc,x
        ora     kk
        sta     rc,x
        dex
        bpl     @or
@chars: lda     pc              ; 1 + 2 * column
        asl     a
        sta     tmp4
        inc     tmp4
        lda     pr
        asl     a
        asl     a
        clc
        adc     #ROW0
        sta     band            ; (the first row, for now)
        ldx     #0
@row:   stx     kk
        txa
        clc
        adc     band
        tay
        lda     #1
        sta     _zdirty,y
        tya
        jsr     rowaddr
        ldx     kk
        ldy     tmp4
        lda     lc,x
        sta     (ptr1),y
        iny
        lda     rc,x
        sta     (ptr1),y
        inx
        cpx     #4
        bcc     @row
        ; the store
        ldy     #0
        lda     (ptr2),y
        beq     @mtn
        jsr     store
@mtn:   ldy     #0              ; mountains (none on the store or river)
        lda     (ptr2),y
        iny
        ora     (ptr2),y
        bne     @m0
        iny
        lda     (ptr2),y
        .byte   $2C             ; (skip the lda #0)
@m0:    lda     #0
        ldx     pp
        cmp     plot_mtn,x
        beq     @done
        sta     plot_mtn,x
        jsr     mountains
@done:  jmp     incsp1

; A = plot (pr, pc)'s left edge: 8 + 16 * column
plotx:  lda     pc
        asl     a
        asl     a
        asl     a
        asl     a
        clc
        adc     #8
        rts

; The store over its plot's four rows (double width, black: colour 1 of P4),
; in place of the river there
store:  lda     #0
        sta     cnt
@z:     lda     pr
        asl     a
        asl     a
        clc
        adc     #ROW0
        adc     cnt
        tay                     ; the zone
        lda     #1
        sta     _zdirty,y
        lda     #TR_RIV
        jsr     tridx
        lda     #0
        sta     _tr_pw,x
        lda     #TR_STORE
        jsr     tridx
        lda     cnt
        asl     a
        asl     a
        adc     #COL_STORE
        sta     _tr_lo,x
        lda     #(4 << 5) | ((-4) & $1F)
        sta     _tr_pw,x
        jsr     plotx
        sta     _tr_x,x
        inc     cnt
        lda     cnt
        cmp     #4
        bcc     @z
        rts

; The mountains of plot row pr, one to a display row, each plot's in the
; rows the others leave (as the original's: four to a plot row); grey,
; colour 1 of P5. A mountain sits on its row's line 6 (gfx.c's pieces).
mountains:
        lda     #0
        sta     band
        ldx     #3              ; none yet in its four rows
@clr:   txa
        pha
        lda     pr
        asl     a
        asl     a
        clc
        adc     #ROW0
        sta     tmp3            ; the first row
        pla
        pha
        clc
        adc     tmp3
        tay
        lda     #1
        sta     _zdirty,y
        lda     #TR_MTN
        jsr     tridx
        lda     #0
        sta     _tr_pw,x
        pla
        tax
        dex
        bpl     @clr
        lda     #0
        sta     pc
        lda     pr              ; its first plot
        asl     a
        asl     a
        asl     a
        adc     pr
        sta     pp
@plot:  ldx     pp
        lda     plot_mtn,x
        beq     @nextp
        sta     cnt
        sec
        sbc     #1
        sta     tmp2
        asl     a
        adc     tmp2            ; 3 * (n - 1): its picks
        sta     tmp2
        lda     #0
        sta     kk
@one:   lda     band
        cmp     #4
        bcs     @nextp
        lda     tmp2
        clc
        adc     kk
        tax
        lda     pick,x
        sta     vv              ; the shape
        lda     band
        clc
        adc     tmp3
        tay                     ; its row
        lda     #TR_MTN
        jsr     tridx
        lda     vv
        asl     a
        adc     #COL_MTN
        sta     _tr_lo,x
        lda     #(5 << 5) | ((-2) & $1F)
        sta     _tr_pw,x
        lda     band
        clc
        adc     pp
        and     #3
        tay
        jsr     plotx
        clc
        adc     dx,y
        sta     _tr_x,x
        inc     band
        inc     kk
        dec     cnt
        beq     @nextp
        jmp     @one
@nextp: inc     pp
        inc     pc
        lda     pc
        cmp     #9
        bcs     @end
        jmp     @plot
@end:   rts

; ---- MTB's glyphs -------------------------------------------------------------
; void __fastcall__ mtb_glyph(const uint8_t *src): map glyph index t_code's
; (1-36) 8 lines from src (1bpp, 8 colour clocks) in its three values: code
; 2 * (index + 37 * (value - 1)); code 0 is left blank
_mtb_glyph:
        sta     ptr1
        stx     ptr1+1
        lda     t_code
        sta     kk
glyph3: lda     #0
        sta     vv              ; value - 1
@v:     lda     vv              ; the code: 2 * (index + 37 * v)
        asl     a
        asl     a
        asl     a               ; 8 v
        sta     tmp1
        asl     a
        asl     a               ; 32 v
        clc
        adc     tmp1            ; 40 v
        sec
        sbc     vv
        sbc     vv
        sbc     vv              ; 37 v
        clc
        adc     kk
        asl     a
        sta     ptr2            ; the code (lo)
        lda     #>MTB + 7
        sta     ptr2+1
        ldy     #0
@l:     sty     tmp2
        lda     (ptr1),y
        sta     tmp3
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jsr     @px
        ldy     #0
        sta     (ptr2),y
        lda     tmp3
        and     #$0F
        jsr     @px
        ldy     #1
        sta     (ptr2),y
        dec     ptr2+1
        ldy     tmp2
        iny
        cpy     #8
        bcc     @l
        inc     vv
        lda     vv
        cmp     #3
        bcc     @v
        rts
@px:    sta     tmp4            ; nibble -> the byte in value vv + 1
        lda     vv
        asl     a
        asl     a
        asl     a
        asl     a
        ora     tmp4
        tax
        lda     _expand_v,x
        rts

; The store's fire (draw.s mappic): 8 picture rows at gp, each doubled, as
; map glyphs t_code (lines 0-7) and t_code + 1 (8-15)
_map_glyph:
        ldx     #7
@d:     txa                     ; the 16 lines into a buffer: row y / 2
        lsr     a
        tay
        lda     (gp),y
        sta     fire,x
        dex
        bpl     @d
        ldx     #7
@d2:    txa
        clc
        adc     #8
        lsr     a
        tay
        lda     (gp),y
        sta     fire+8,x
        dex
        bpl     @d2
        ldx     t_code
        lda     _mapidx,x
        bmi     @n1
        sta     kk
        lda     #<fire
        sta     ptr1
        lda     #>fire
        sta     ptr1+1
        jsr     glyph3
@n1:    ldx     t_code
        lda     _mapidx+1,x
        bmi     @n2
        sta     kk
        lda     #<(fire + 8)
        sta     ptr1
        lda     #>(fire + 8)
        sta     ptr1+1
        jsr     glyph3
@n2:    rts

        .bss
fire:   .res 16
        .code

; void __fastcall__ mtb_town(const uint8_t *chars): the town's 128
; characters (ANTIC mode 4: 2bpp, as 160A) as codes 0-127 with value 3
; cleared (the town's PF2 is its background) and 128-255 as they are
; (an inverse character's value 3 is PF3: colour 3 of palette A)
_mtb_town:
        sta     ptr1
        stx     ptr1+1
        lda     #0
        sta     kk              ; the character
@c:     lda     #>MTB + 7
        sta     ptr2+1
        lda     #0
        sta     ptr2
        ldy     #0
@l:     sty     tmp2
        lda     (ptr1),y
        sta     tmp3
        ldy     kk
        sta     (ptr2),y        ; inverse: as is (code + 128)
        tya
        ora     #$80
        tay
        lda     tmp3
        sta     (ptr2),y
        ; normal: value 3 pixels cleared
        lda     tmp3
        and     #$55
        sta     tmp4            ; low bits
        lda     tmp3
        lsr     a
        and     #$55            ; high bits
        and     tmp4            ; both: value 3
        sta     tmp4
        asl     a
        ora     tmp4            ; its mask
        eor     #$FF
        and     tmp3
        ldy     kk
        sta     (ptr2),y
        dec     ptr2+1
        ldy     tmp2
        iny
        cpy     #8
        bcc     @l
        lda     ptr1
        clc
        adc     #8
        sta     ptr1
        bcc     @n
        inc     ptr1+1
@n:     inc     kk
        bpl     @c
        rts
