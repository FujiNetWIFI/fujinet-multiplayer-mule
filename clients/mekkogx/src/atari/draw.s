;
; draw.s -- gfx.h's text and drawing on the Atari's rows (gfx.c has the
; screens; cells.s the drawing rows' characters; mapgfx.s the map).
;
; A row is in one mode (gfx.c: RM_*): text (mode 2), big (mode 6), draw
; (mode 4 cells), map (mode 6, map characters), town (mode 4, town ones).
; Big text on a text row that has small text, or small on a big row, turns
; the row into a drawing row: big characters are exactly two mode 4 cells,
; small ones are squeezed into one.
;
        .export _gfx_textn, _gfx_text, _gfx_char, _gfx_big, _gfx_bign
        .export _gfx_clear_text, _gfx_fill, _gfx_hdots, _gfx_pic
        .export _spr_set, _spr_hide, _spr_hide_all
        .export _row_setmode, _row_switch, _dl_build
        .import _row_mode, _row_pf, _row_chb, _row_cm, _row_used, _row_lo, _row_hi
        .import _cp_r, _cp_cx, _cp_line, _cp_mask, _cp_col
        .import _cell_put, _row_slot, _dyn_release
        .import _atari_col, _pf_dirty, _sp_dirty, _text_bg, _text_fg, _scr_kind
        .import _pics, _mbar, _mbar_col, _map_bar, _map_slot, _map_pr, _mp_uses
        .import _dsp_hposm, _dsp_sizem
        .import _spr_on, _spr_shape, _spr_frame, _spr_xl, _spr_xh, _spr_yl, _spr_yh
        .import _spr_col, _spr_flags, _spr_changed
        .import incsp1, incsp2, incsp3, incsp4, incsp5, incsp8
        .importzp c_sp

ROWS    = 24
SCREEN  = $B400
DLIST   = $B800
FONT    = $AC00
MAPCH   = $AE00
MISSILES = $BB00
RM_TEXT = 0
RM_BIG  = 1
RM_DRAW = 2
RM_MAP  = 3
RM_TOWN = 4
SK_MAP  = 1
C_BLACK = 0
C_WHITE = 1
TOWN_ROW0 = 2
CORRAL_ROW = TOWN_ROW0 + 11
MAP_ROW0 = 2
SLOTS   = 10

        .segment "ZEROPAGE"
sptr:   .res 2          ; the string
rptr:   .res 2          ; the row
gp:     .res 2          ; a glyph

        .segment "BSS2"         ; low memory (mule-atari.cfg)
t_x:    .res 1
t_y:    .res 1
t_n:    .res 1
t_col:  .res 1
t_big:  .res 1
t_mode: .res 1
t_slot: .res 1
t_code: .res 1
t_i:    .res 1
t_w:    .res 1
t_h:    .res 1
t_c0:   .res 1
t_c1:   .res 1
t_x1:   .res 1
t_b:    .res 1
t_sw:   .res 1
cbuf:   .res 2
buf:    .res 80             ; a row, and (to_big) where its characters go
pal:    .res 4

        .rodata
rm_antic: .byte $C2, $C6, $C4, $C6, $C4     ; LMS + DLI + the mode
rm_chb:   .byte $AC, $AC, $A8, $AE, $B0
nib_mask: .byte $00, $03, $0C, $0F, $30, $33, $3C, $3F
          .byte $C0, $C3, $CC, $CF, $F0, $F3, $FC, $FF
left_m:   .byte $FF, $3F, $0F, $03
right_m:  .byte $C0, $F0, $FC, $FF
px_m:     .byte $C0, $30, $0C, $03

        .code

; ---- rows -------------------------------------------------------------------
; void __fastcall__ row_setmode(uint8_t r, uint8_t m)
_row_setmode:
        pha
        ldy     #0
        lda     (c_sp),y
        tax
        pla
        jsr     setmode
        jmp     incsp1

; X = row, A = mode (X kept)
setmode:
        sta     _row_mode,x
        tay
        lda     rm_chb,y
        sta     _row_chb,x
        lda     #0
        sta     _row_used,x
        sta     _row_pf,x
        sta     _row_pf+ROWS*3,x
        sta     _row_pf+ROWS,x
        sta     _row_pf+ROWS*2,x
        cpy     #RM_TEXT
        bne     @d
        lda     _text_fg
        sta     _row_pf+ROWS,x
        lda     _text_bg
        sta     _row_pf+ROWS*2,x
@d:     lda     #1
        sta     _pf_dirty
        rts

; void __fastcall__ row_switch(uint8_t r, uint8_t m)
_row_switch:
        pha
        ldy     #0
        lda     (c_sp),y
        tax
        pla
        jsr     switch
        jmp     incsp1

; X = row, A = mode: the row, blank, in that mode (unless it is already)
switch: cmp     _row_mode,x
        beq     @r
        sta     t_sw
        jsr     rowptr
        lda     #0
        ldy     #39
@z:     sta     (rptr),y
        dey
        bpl     @z
        lda     t_sw
        jsr     setmode
        stx     t_i
        txa
        asl     a
        adc     t_i             ; 3 * row
        tay
        ldx     t_sw
        lda     rm_antic,x
        cpy     #3 * (ROWS - 1)
        bne     @dl
        and     #$7F            ; no DLI after the last row
@dl:    sta     DLIST+3,y
        ldx     t_i
@r:     rts

; X = row -> rptr (X kept)
rowptr: lda     _row_lo,x
        sta     rptr
        lda     _row_hi,x
        sta     rptr+1
        rts

; void dl_build(void): 24 blank lines, the rows, back
_dl_build:
        lda     #$70
        sta     DLIST
        sta     DLIST+1
        sta     DLIST+2
        ldx     #0
        ldy     #3
@row:   lda     _row_mode,x
        stx     t_i
        tax
        lda     rm_antic,x
        ldx     t_i
        cpx     #ROWS - 1
        bne     @n
        and     #$7F
@n:     sta     DLIST,y
        lda     _row_lo,x
        sta     DLIST+1,y
        lda     _row_hi,x
        sta     DLIST+2,y
        iny
        iny
        iny
        inx
        cpx     #ROWS
        bcc     @row
        lda     #$41
        sta     DLIST,y
        lda     #<DLIST
        sta     DLIST+1,y
        lda     #>DLIST
        sta     DLIST+2,y
        rts

; ---- text -------------------------------------------------------------------
; void gfx_textn(uint8_t tx, uint8_t ty, const char *s, uint8_t n, uint8_t col)
_gfx_bign:
        ldy     #1
        .byte   $2C
_gfx_textn:
        ldy     #0
        sty     t_big
        sta     t_col
        ldy     #0
        lda     (c_sp),y
        sta     t_n
        iny
        lda     (c_sp),y
        sta     sptr
        iny
        lda     (c_sp),y
        sta     sptr+1
        iny
        lda     (c_sp),y
        sta     t_y
        iny
        lda     (c_sp),y
        sta     t_x
        jsr     text
        jmp     incsp5

; void gfx_text(uint8_t tx, uint8_t ty, const char *s, uint8_t col)
_gfx_big:
        ldy     #1
        .byte   $2C
_gfx_text:
        ldy     #0
        sty     t_big
        sta     t_col
        lda     #127
        sta     t_n
        ldy     #0
        lda     (c_sp),y
        sta     sptr
        iny
        lda     (c_sp),y
        sta     sptr+1
        iny
        lda     (c_sp),y
        sta     t_y
        iny
        lda     (c_sp),y
        sta     t_x
        jsr     text
        jmp     incsp4

; void gfx_char(uint8_t tx, uint8_t ty, char c, uint8_t col)
_gfx_char:
        sta     t_col
        ldy     #0
        sty     t_big
        lda     (c_sp),y
        sta     cbuf
        sty     cbuf+1
        iny
        lda     (c_sp),y
        sta     t_y
        iny
        lda     (c_sp),y
        sta     t_x
        lda     #1
        sta     t_n
        lda     #<cbuf
        sta     sptr
        lda     #>cbuf
        sta     sptr+1
        jsr     text
        jmp     incsp3

; the text at sptr (t_n characters at most) at t_x, t_y in t_col; t_big
text:   ldx     t_y
        cpx     #ROWS
        bcc     @ok
        rts
@ok:    lda     t_big
        beq     @small
        lda     t_x
        and     #$FE
        sta     t_x
        lda     _row_mode,x
        bne     @mode           ; (big on a text row:)
        jsr     rowptr
        ldy     #39
@e:     lda     (rptr),y
        bne     @full
        dey
        bpl     @e
        lda     #RM_BIG         ; an empty row becomes a big one
        jsr     switch
        jmp     @mode
@full:  jsr     to_big          ; one with text: big if its words fit,
        bcc     @mode           ; else a drawing row
        jsr     to_draw
        jmp     @mode
@small: lda     _row_mode,x
        cmp     #RM_BIG
        bne     @town
        jsr     fits            ; room for it as big text: draw it so
        bcc     @mode           ; (a drawing row costs characters)
        jsr     to_draw
        jmp     @mode
@town:  cmp     #RM_TOWN
        bne     @mode
        ; the town has no letters: the corral's count is its M.U.L.E.s,
        ; anything else goes under the town
        cpx     #CORRAL_ROW
        bne     @under
        jmp     corral
@under: ldx     #TOWN_ROW0 + 19
        stx     t_y
@mode:  ldx     t_y
        stx     _cp_r
        lda     _row_mode,x
        sta     t_mode
        jsr     rowptr
        lda     t_mode
        cmp     #RM_BIG
        beq     @slot
        cmp     #RM_MAP
        bne     @loop
@slot:  lda     t_col
        jsr     _row_slot
        lsr     a
        ror     a
        ror     a
        sta     t_slot
@loop:  lda     t_n
        beq     @done
        ldy     #0
        lda     (sptr),y
        beq     @done
        ldx     t_x
        cpx     #40
        bcs     @done
        jsr     internal
        sta     t_code
        inc     sptr
        bne     @s2
        inc     sptr+1
@s2:    lda     t_mode
        cmp     #RM_BIG
        beq     @b6
        cmp     #RM_MAP
        beq     @b6
        cmp     #RM_DRAW
        beq     @cell
        ldy     t_x             ; text, town: the code
        lda     t_code
        sta     (rptr),y
        jmp     @next
@b6:    lda     t_x             ; mode 6: a character is two cells
        lsr     a
        tay
        lda     t_code
        beq     @b0
        and     #$3F
        ora     t_slot
@b0:    sta     (rptr),y
        inc     t_x
        jmp     @next
@cell:  lda     t_big
        beq     @sq
        jsr     big_cells
        inc     t_x
        jmp     @next
@sq:    jsr     text_cell
@next:  inc     t_x
        dec     t_n
        jmp     @loop
@done:  rts

; Carry clear if the small text at sptr fits on big row X as big text:
; from t_x / 2, over characters that are blank. (It is then drawn big.)
fits:   jsr     rowptr
        lda     t_x
        lsr     a
        tay
        sty     t_i
        ldy     #0
@len:   lda     (sptr),y
        beq     @l
        iny
        cpy     t_n
        bcc     @len
@l:     tya
        clc
        adc     t_i
        cmp     #21
        bcs     @no             ; past the row's end
        sta     t_c1
        ldy     t_i
@chk:   cpy     t_c1
        bcs     @yes
        lda     (rptr),y        ; blank, or the same character already
        beq     @ok
        and     #$3F
        sta     t_b
        tya
        sec
        sbc     t_i
        sty     t_w
        tay
        lda     (sptr),y
        jsr     internal
        ldy     t_w
        cmp     t_b
        bne     @no
@ok:    iny
        bne     @chk
@yes:   lda     t_x
        and     #$FE
        sta     t_x
        clc
        rts
@no:    sec
        rts

; A: ASCII -> the font's code (upper case; 0 for what it hasn't)
internal:
        cmp     #'a'
        bcc     @u
        cmp     #'z' + 1
        bcs     @u
        and     #$DF
@u:     sec
        sbc     #32
        cmp     #64
        bcc     @r
        lda     #0
@r:     rts

; gp = the glyph of t_code
glyph:  lda     t_code
        ldx     #0
        stx     gp+1
        asl     a
        asl     a
        asl     a
        rol     gp+1
        sta     gp
        lda     gp+1
        adc     #>FONT
        sta     gp+1
        rts

; t_code squeezed into cell t_x of row _cp_r (every other pixel: the
; font's strokes are two wide), colour t_col
text_cell:
        jsr     glyph
        lda     t_x
        sta     _cp_cx
        lda     #0
        sta     _cp_line
@line:  ldy     _cp_line
        lda     (gp),y
        sta     t_b
        lda     #$FF
        sta     _cp_mask
        lda     #C_BLACK
        sta     _cp_col
        jsr     _cell_put
        ldx     #3
        lda     #0
@px:    lsr     t_b             ; bits 0, 2, 4, 6 -> pixels 3, 2, 1, 0
        bcc     @n
        ora     px_m,x
@n:     lsr     t_b
        dex
        bpl     @px
        cmp     #0
        beq     @nl
        sta     _cp_mask
        lda     t_col
        sta     _cp_col
        jsr     _cell_put
@nl:    inc     _cp_line
        lda     _cp_line
        cmp     #8
        bcc     @line
        rts

; t_code as a big character: cells t_x and t_x + 1 of row _cp_r
big_cells:
        jsr     glyph
        lda     #0
        sta     _cp_line
@line:  ldy     _cp_line
        lda     (gp),y
        sta     t_b
        ldx     t_x
        stx     _cp_cx
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jsr     @half
        inc     _cp_cx
        lda     t_b
        and     #15
        jsr     @half
        inc     _cp_line
        lda     _cp_line
        cmp     #8
        bcc     @line
        rts
@half:  tax
        lda     nib_mask,x
        pha
        lda     #$FF
        sta     _cp_mask
        lda     #C_BLACK
        sta     _cp_col
        jsr     _cell_put
        pla
        beq     @r
        sta     _cp_mask
        lda     t_col
        sta     _cp_col
        jsr     _cell_put
@r:     rts

; Row X becomes a drawing row, keeping what it shows
to_draw:
        stx     _cp_r
        jsr     rowptr
        lda     _row_mode,x
        sta     t_mode
        ldy     #39
@c:     lda     (rptr),y
        sta     buf,y
        dey
        bpl     @c
        ldy     #3
@p:     txa
        clc
        adc     pfoff,y
        tax
        lda     _row_pf,x
        sta     pal,y
        ldx     _cp_r
        dey
        bpl     @p
        lda     #RM_DRAW
        jsr     switch
        lda     t_col           ; (the text's colour: kept)
        pha
        lda     #0
        sta     t_i
@ch:    ldx     t_i
        lda     buf,x
        beq     @nx
        sta     t_b
        and     #$3F
        sta     t_code
        lda     t_x             ; (kept too)
        pha
        lda     t_mode
        cmp     #RM_BIG
        bne     @sm
        cpx     #20
        bcs     @skip
        txa
        asl     a
        sta     t_x
        lda     t_b             ; the colour of its slot
        rol     a
        rol     a
        rol     a
        and     #3
        tax
        lda     pal,x
        jsr     col_index
        sta     t_col
        jsr     big_cells
        jmp     @skip
@sm:    stx     t_x
        lda     t_b
        sta     t_code
        lda     #C_WHITE
        sta     t_col
        jsr     text_cell
@skip:  pla
        sta     t_x
@nx:    inc     t_i
        lda     t_i
        cmp     #40
        bcc     @ch
        pla
        sta     t_col
        rts

pfoff:  .byte   0, ROWS, ROWS*2, ROWS*3

; Text row X as a big one, if its words fit: each word from its column / 2,
; a character a cell, none past the row or on another. Carry clear if done.
to_big: stx     _cp_r
        jsr     rowptr
        ldy     #39
@c:     lda     (rptr),y
        sta     buf,y
        dey
        bpl     @c
        ; where each character goes (t_b: the next free cell)
        ldx     #0
        stx     t_b
        dex                     ; X = $FF: no word
        ldy     #0
@pos:   lda     buf,y
        beq     @gap
        cpx     #$FF
        bne     @in
        tya                     ; a word starts: at its column / 2
        lsr     a
        cmp     t_b
        bcc     @no             ; on the last word
        tax
@in:    cpx     #20
        bcs     @no
        txa
        sta     buf+40,y        ; (pal, and past: a second 40 bytes)
        inx
        stx     t_b
        jmp     @nx
@gap:   ldx     #$FF
@nx:    iny
        cpy     #40
        bcc     @pos
        ; it fits: make it big, in white
        ldx     _cp_r
        lda     #RM_BIG
        jsr     switch
        lda     #C_WHITE
        jsr     _row_slot
        lsr     a
        ror     a
        ror     a
        sta     t_b
        ldx     #0
@put:   lda     buf,x
        beq     @n2
        ldy     buf+40,x
        and     #$3F
        ora     t_b
        sta     (rptr),y
@n2:    inx
        cpx     #40
        bcc     @put
        ldx     _cp_r
        clc
        rts
@no:    ldx     _cp_r
        sec
        rts

; A = colour register value -> gfx.h colour
col_index:
        ldx     #15
@f:     cmp     _atari_col,x
        beq     @r
        dex
        bne     @f
@r:     txa
        rts

; The corral: sptr's number of M.U.L.E.s (town characters $7B-$7C, inverse),
; up to 14, two columns of seven
corral: lda     #0
        sta     t_n
        tay
@d:     lda     (sptr),y
        beq     @go
        iny
        sec
        sbc     #'0'
        cmp     #10
        bcs     @d
        sta     t_b
        lda     t_n
        asl     a
        asl     a
        adc     t_n
        asl     a
        adc     t_b
        sta     t_n
        jmp     @d
@go:    lda     #0
        sta     t_i
@m:     lda     t_i
        ldx     #TOWN_ROW0 + 10
        ldy     #26
@r7:    cmp     #7
        bcc     @at
        sbc     #7
        ldy     #28
@at:    stx     t_b
        clc
        adc     t_b
        tax
        jsr     rowptr
        lda     t_i
        cmp     t_n
        lda     #0
        ldx     #0
        bcs     @put
        lda     #$FB
        ldx     #$FC
@put:   sta     (rptr),y
        iny
        txa
        sta     (rptr),y
        inc     t_i
        lda     t_i
        cmp     #14
        bcc     @m
        rts

; void gfx_clear_text(uint8_t tx, uint8_t ty, uint8_t w)
_gfx_clear_text:
        sta     t_w
        ldy     #0
        lda     (c_sp),y
        sta     t_y
        iny
        lda     (c_sp),y
        sta     t_x
        jsr     @go
        jmp     incsp2
@go:    ldx     t_y
        cpx     #ROWS
        bcs     @r
        lda     t_x
        cmp     #40
        bcs     @r
        clc
        adc     t_w
        cmp     #41
        bcc     @w
        lda     #40
@w:     sta     t_x1            ; the end
        jsr     rowptr
        lda     _row_mode,x
        cmp     #RM_MAP
        beq     @r
        cmp     #RM_BIG
        bne     @dr
        lda     t_x1            ; big: in characters
        clc
        adc     #1
        lsr     a
        sta     t_x1
        lsr     t_x
@dr:    ldy     t_x
@z:     cpy     t_x1
        bcs     @r
        lda     _row_mode,x
        cmp     #RM_DRAW
        bne     @0
        lda     (rptr),y
        sty     t_i
        jsr     _dyn_release
        ldy     t_i
        ldx     t_y
@0:     lda     #0
        sta     (rptr),y
        iny
        bne     @z
@r:     rts

; ---- drawing ----------------------------------------------------------------
; void gfx_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t col)
_gfx_fill:
        sta     t_col
        ldy     #0
        lda     (c_sp),y
        sta     t_h
        iny
        lda     (c_sp),y
        sta     t_w
        iny
        lda     (c_sp),y
        sta     t_y
        iny
        lda     (c_sp),y
        sta     t_x
        jsr     fill
        jmp     incsp4

fill:   lda     t_w
        bne     @fx0_7
        jmp     @r
@fx0_7:
        lda     t_h
        bne     @fx0_6
        jmp     @r
@fx0_6:
        lda     t_x
        lsr     a
        lsr     a
        sta     t_c0
        lda     t_x
        clc
        adc     t_w
        sec
        sbc     #1
        sta     t_x1
        lsr     a
        lsr     a
        sta     t_c1
@line:  lda     t_y
        cmp     #192
        bcc     @fx0_5
        jmp     @r
@fx0_5:
        lsr     a
        lsr     a
        lsr     a
        sta     _cp_r
        tax
        lda     t_y
        and     #7
        sta     _cp_line
        lda     _row_mode,x
        cmp     #RM_MAP
        bne     @nm
        jsr     mapbar
        jmp     @next
@nm:    cmp     #RM_TOWN
        bne     @dr
        jsr     missile
        jmp     @next
@dr:    lda     #RM_DRAW
        jsr     switch
        lda     t_col
        sta     _cp_col
        lda     t_c0
        sta     _cp_cx
@cell:  lda     _cp_cx
        cmp     #40
        bcs     @next
        lda     #$FF
        ldx     _cp_cx
        cpx     t_c0
        bne     @nl
        lda     t_x
        and     #3
        tax
        lda     left_m,x
@nl:    ldx     _cp_cx
        cpx     t_c1
        bne     @nr
        sta     t_b
        lda     t_x1
        and     #3
        tax
        lda     right_m,x
        and     t_b
@nr:    sta     _cp_mask
        jsr     _cell_put
        lda     _cp_cx
        inc     _cp_cx
        cmp     t_c1
        bcc     @cell
@next:  inc     t_y
        dec     t_h
        beq     @fx0_4
        jmp     @line
@fx0_4:
@r:     rts

; the time bar on a map row: lines lit (bit 7 the top), then its character
mapbar: ldy     _cp_line
        lda     bitr,y
        ldx     _cp_r
        ldy     t_col
        beq     @off
        ora     _mbar,x
        sta     _mbar,x
        sty     _mbar_col
        jmp     @chr
@off:   eor     #$FF
        and     _mbar,x
        sta     _mbar,x
@chr:   lda     _cp_line
        cmp     #7
        beq     @go
        lda     t_h
        cmp     #1
        bne     @r
@go:    lda     _cp_r
        jmp     _map_bar
@r:     rts

bitr:   .byte   $80, $40, $20, $10, $08, $04, $02, $01

; the time bar where the row can't draw (the town): missile 3, 4 wide,
; coloured like player 3
missile:
        lda     t_x
        clc
        adc     #48
        sta     _dsp_hposm+3
        lda     #$40
        sta     _dsp_sizem
        ldx     t_y
        lda     MISSILES+32,x
        ldy     t_col
        beq     @off
        ora     #$C0
        sta     MISSILES+32,x
        lda     _atari_col,y
        ldx     #ROWS - 1
@c:     sta     _row_cm+ROWS*3,x
        dex
        bpl     @c
        lda     #1
        sta     _sp_dirty
        rts
@off:   and     #$3F
        sta     MISSILES+32,x
        rts

; void gfx_hdots(uint8_t x, uint8_t y, uint8_t w, uint8_t col): a dotted line
_gfx_hdots:
        sta     _cp_col
        ldy     #0
        lda     (c_sp),y
        sta     t_w
        iny
        lda     (c_sp),y
        sta     t_y
        iny
        lda     (c_sp),y
        sta     t_x
        jsr     @go
        jmp     incsp3
@go:    lda     t_y
        cmp     #192
        bcs     @r
        lsr     a
        lsr     a
        lsr     a
        sta     _cp_r
        tax
        lda     t_y
        and     #7
        sta     _cp_line
        lda     #RM_DRAW
        jsr     switch
        lda     t_x
        clc
        adc     t_w
        sta     t_x1
@dot:   lda     t_x
        cmp     t_x1
        bcs     @r
        cmp     #160
        bcs     @r
        lsr     a
        lsr     a
        sta     _cp_cx
        lda     t_x
        and     #3
        tax
        lda     px_m,x
        sta     _cp_mask
        jsr     _cell_put
        inc     t_x
        inc     t_x
        bne     @dot
@r:     rts

; void gfx_pic(uint8_t x, uint8_t y, uint8_t pic, uint8_t col): 8 x 8, two
; scanlines a row (on the map: characters $37-$3E over the store)
_gfx_pic:
        sta     t_col
        ldy     #0
        lda     (c_sp),y
        and     #3
        asl     a
        asl     a
        asl     a
        clc
        adc     #<_pics
        sta     gp
        lda     #>_pics
        adc     #0
        sta     gp+1
        iny
        lda     (c_sp),y
        sta     t_y
        iny
        lda     (c_sp),y
        sta     t_x
        lda     _scr_kind
        cmp     #SK_MAP
        beq     @map
        jsr     pic
        jmp     incsp3
@map:   jsr     mappic
        jmp     incsp3

pic:    lda     #0
        sta     t_w             ; (switch uses t_i)
@row:   ldy     t_w
        lda     (gp),y
        sta     t_b
        jsr     bits
        inc     t_y
        jsr     bits
        inc     t_y
        inc     t_w
        lda     t_w
        cmp     #8
        bcc     @row
        rts

; the 8 colour clocks of t_b at t_x, t_y: set ones t_col, the rest black
bits:   lda     t_y
        cmp     #192
        bcs     @r
        lsr     a
        lsr     a
        lsr     a
        sta     _cp_r
        tax
        lda     t_y
        and     #7
        sta     _cp_line
        lda     #RM_DRAW
        jsr     switch
        lda     t_x
        sta     t_x1
        lda     t_b
        sta     t_c0
        ldy     #8
@px:    lda     t_x1
        cmp     #160
        bcs     @r
        lsr     a
        lsr     a
        sta     _cp_cx
        lda     t_x1
        and     #3
        tax
        lda     px_m,x
        sta     _cp_mask
        lda     #C_BLACK
        asl     t_c0
        bcc     @blk
        lda     t_col
@blk:   sta     _cp_col
        sty     t_c1
        jsr     _cell_put
        ldy     t_c1
        inc     t_x1
        dey
        bne     @px
@r:     rts

; the store's fire on the map: a picture as two map characters, in the
; plot row's colour slot for t_col
mappic: lda     t_y
        lsr     a
        lsr     a
        lsr     a
        sta     t_i             ; the row
        cmp     #MAP_ROW0
        bcs     @fx0_3
        jmp     @r
@fx0_3:
        cmp     #MAP_ROW0 + 19
        bcc     @fx0_2
        jmp     @r
@fx0_2:
        lda     t_x
        lsr     a
        lsr     a
        lsr     a
        sta     t_c0            ; the column
        cmp     #20
        bcc     @fx0_1
        jmp     @r
@fx0_1:
        lda     t_i
        sec
        sbc     #MAP_ROW0
        lsr     a
        lsr     a
        sta     _map_pr
        lda     t_col
        jsr     _map_slot
        sta     t_slot
        lda     _map_pr         ; (a picture doesn't hold its colour)
        asl     a
        asl     a
        adc     t_slot
        tax
        dec     _mp_uses,x
        lda     t_slot
        lsr     a
        ror     a
        ror     a
        sta     t_slot
        ; characters $37 + 2 * (column & 1 | row & 2)
        lda     t_i
        and     #2
        sta     t_b
        lda     t_c0
        and     #1
        ora     t_b
        asl     a
        adc     #$37
        sta     t_code
        ; its 16 lines: the picture's rows doubled
        ldx     #0
        stx     rptr+1
        asl     a
        rol     rptr+1
        asl     a
        rol     rptr+1
        asl     a
        rol     rptr+1
        sta     rptr
        lda     rptr+1
        clc
        adc     #>MAPCH
        sta     rptr+1
        ldy     #15
@l:     tya
        lsr     a
        sty     t_b
        tay
        lda     (gp),y
        ldy     t_b
        sta     (rptr),y
        dey
        bpl     @l
        ldx     t_i
        jsr     rowptr
        ldy     t_c0
        lda     t_code
        ora     t_slot
        sta     (rptr),y
        inx
        jsr     rowptr
        lda     t_code
        clc
        adc     #1
        ora     t_slot
        sta     (rptr),y
@r:     rts

; ---- sprites ------------------------------------------------------------------
; void spr_set(uint8_t slot, uint8_t shape, uint8_t frame, int16_t x, int16_t y,
;              uint8_t col, uint8_t flags)
_spr_set:
        sta     t_b             ; flags
        ldy     #7
        lda     (c_sp),y
        tax                     ; the slot
        ldy     #0
        lda     (c_sp),y
        tay
        lda     _atari_col,y    ; the colour register value
        sta     t_c0
        lda     _spr_on,x
        beq     @set
        lda     t_b
        cmp     _spr_flags,x
        bne     @set
        lda     t_c0
        cmp     _spr_col,x
        bne     @set
        ldy     #1
        lda     (c_sp),y
        cmp     _spr_yl,x
        bne     @set
        iny
        lda     (c_sp),y
        cmp     _spr_yh,x
        bne     @set
        iny
        lda     (c_sp),y
        cmp     _spr_xl,x
        bne     @set
        iny
        lda     (c_sp),y
        cmp     _spr_xh,x
        bne     @set
        iny
        lda     (c_sp),y
        cmp     _spr_frame,x
        bne     @set
        iny
        lda     (c_sp),y
        cmp     _spr_shape,x
        beq     @same
@set:   lda     t_b
        sta     _spr_flags,x
        lda     t_c0
        sta     _spr_col,x
        ldy     #1
        lda     (c_sp),y
        sta     _spr_yl,x
        iny
        lda     (c_sp),y
        sta     _spr_yh,x
        iny
        lda     (c_sp),y
        sta     _spr_xl,x
        iny
        lda     (c_sp),y
        sta     _spr_xh,x
        iny
        lda     (c_sp),y
        sta     _spr_frame,x
        iny
        lda     (c_sp),y
        sta     _spr_shape,x
        lda     #1
        sta     _spr_on,x
        sta     _spr_changed
@same:  jmp     incsp8

; void __fastcall__ spr_hide(uint8_t slot)
_spr_hide:
        tax
        lda     _spr_on,x
        beq     @r
        lda     #0
        sta     _spr_on,x
        lda     #1
        sta     _spr_changed
@r:     rts

; void spr_hide_all(void)
_spr_hide_all:
        ldx     #SLOTS - 1
        lda     #0
@c:     sta     _spr_on,x
        dex
        bpl     @c
        lda     #1
        sta     _spr_changed
        rts
