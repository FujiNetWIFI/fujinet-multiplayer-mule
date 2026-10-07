;
; hires.s -- HGR page 1 primitives for M.U.L.E. (Apple II).
;
; Arguments go in the hg_ variables (set from C), then a routine is called.
; Screen bytes are addressed by byte column (0..39) and line (0..191).
;
;   hgr_init       HGR page 1, full screen, cleared
;   hgr_clear      clear lines hg_y .. hg_y + hg_h - 1 (whole width)
;   hgr_fill       hg_h lines x hg_n bytes at hg_bx, hg_y: even columns get
;                  hg_v0, odd columns hg_v1 (colour patterns alternate)
;   hgr_glyph      8 lines of the glyph at hg_src, ANDed with hg_mask (even
;                  column) / hg_mask1 (odd), ORed with hg_hi, at hg_bx, hg_y
;   hgr_glyph2     the same, twice as wide (2 bytes: each glyph pixel is a
;                  colour pixel), at the even column hg_bx
;   hgr_blit       hg_h lines x hg_n bytes from hg_src to the screen
;   hgr_save       the same rectangle from the screen to hg_src
;   hgr_plot       a map plot, 4 bytes x 32 lines at hg_bx, hg_y, built from
;                  128-byte layers: pl_own1|2|3 in the owner's colour
;                  (pl_om: its bits in each of the 4 bytes, palette pl_hi),
;                  pl_white as it is, pl_river in blue where the owner's
;                  layer leaves the byte free (point unused layers at zeros)
;   hgr_spr        a 1-bit sprite (hg_src, hg_wb bytes a row, hg_h lines)
;                  with its left colour pixel at hg_x - 64 (0..139 shows),
;                  line hg_y, in colour hg_col (0 black, 1 white, 2 violet,
;                  3 green, 4 blue, 5 orange)
;   spr_lines      spr_flush's line loop (gfx.c): for each line, the old
;                  rows put back, the new ones saved and drawn
;

        .include        "apple2.inc"

        .import         _frame_tick     ; lowlevel.s: counted once a row

        .export         _hgr_init, _hgr_clear, _hgr_fill, _hgr_glyph, _hgr_glyph2
        .export         _hgr_blit, _hgr_save, _hgr_spr, _hgr_plot
        .export         _pl_own1, _pl_own2, _pl_own3, _pl_white, _pl_river
        .export         _pl_om, _pl_hi
        .export         _hg_bx, _hg_y, _hg_h, _hg_n, _hg_src
        .export         _hg_mask, _hg_mask1, _hg_hi, _hg_v0, _hg_v1
        .export         _hg_x, _hg_wb, _hg_col
        .export         _hgr_rowlo, _hgr_rowhi, _hgr_cxb0, _hgr_cxb1
        .export         _spr_lines, _spr_one, _sl_cl, _sl_nc, _sl_y0, _sl_y1, _sl_pass
        .export         _src_of, _spr_groups, _d_no0, _g_cl, _g_nc, _g_y0, _g_y1, _g_pass
        .import         _s_on, _s_shape, _s_frame, _s_flags, _s_col
        .import         _s_xl, _s_xh, _s_yl, _s_yh, _hcol, _hgr_shapes, _d_rows
        .import         _s_drawn, _s_sy, _s_sh, _s_bx, _s_n, _s_st, _hit
        .import         _d_oo, _d_oend, _d_show, _d_sy, _d_sh, _d_bx, _d_n, _d_st
        .import         _d_no, _d_nend, _d_wb, _d_col, _d_cx, _d_fast, _d_src
        .import         _save_at, _lift_y0, _lift_y1

        .zeropage
dst:            .res 2
_hg_src:        .res 2
_pl_own1:       .res 2              ; hgr_plot's layers (128 bytes each)
_pl_own2:       .res 2
_pl_own3:       .res 2
_pl_white:      .res 2
_pl_river:      .res 2

        .bss
_hg_bx:         .res 1
_hg_y:          .res 1
_hg_h:          .res 1
_hg_n:          .res 1
_hg_mask:       .res 1
_hg_mask1:      .res 1
_hg_hi:         .res 1
_hg_v0:         .res 1
_hg_v1:         .res 1
_hg_x:          .res 1
_hg_wb:         .res 1
_hg_col:        .res 1
_pl_om:         .res 4              ; the owner's colour bits, per byte of the row
_pl_hi:         .res 1
ownv:           .res 1
line:           .res 1
cnt:            .res 1
col:            .res 1
bytes:          .res 1
bits:           .res 1
curx:           .res 1                  ; drawrow: the colour pixel (+ 64)
pxc:            .res 1                  ; drawrow: the same, as it goes
fastf:          .res 1                  ; drawrow: the row is all on the screen
kk:             .res 1                  ; spr_lines: the place in sl_cl
slot:           .res 1
fbuf:           .res 64                 ; a picture's rows, from aux RAM
_sl_cl:         .res 10                 ; spr_lines: the slots, in order
_sl_nc:         .res 1
_sl_y0:         .res 1
_sl_y1:         .res 1
_sl_pass:       .res 1                  ; 0 put back, 1 save and draw, 2 both
lastcol:        .res 1                  ; spr_lines: the colour setcol has
; spr_one's sprite, old and new
sbase:          .res 2
o_on:           .res 1
o_y0:           .res 1
o_y1:           .res 1
o_bx:           .res 1
o_n:            .res 1
o_st:           .res 1
o_end:          .res 1
o_lift:         .res 1
oo:             .res 1
n_on:           .res 1
n_y0:           .res 1
n_y1:           .res 1
n_bx:           .res 1
n_n:            .res 1
n_st:           .res 1
n_end:          .res 1
no:             .res 1
wbm1:           .res 1
cxs:            .res 1
fsts:           .res 1
same:           .res 1
putl:           .res 1
; src_of
frm:            .res 1
shp:            .res 1
srcl:           .res 1
srch:           .res 1
yy:             .res 1
hh:             .res 1
cxv:            .res 1
leftc:          .res 1
rightc:         .res 1
mcand:          .res 1
mplier:         .res 1
mlo:            .res 1
tl:             .res 1
; spr_groups
_g_cl:          .res 10                 ; the flush's slots, in order
_g_nc:          .res 1
_g_y0:          .res 1
_g_y1:          .res 1
_g_pass:        .res 1
glo:            .res 10                 ; each one's lines and bytes, old and new
ghi:            .res 10
gxl:            .res 10
gxh:            .res 10
glab:           .res 10
_d_no0:         .res 10                 ; src_of: d_no as it set it
wlo:            .res 10                 ; spr_lines: each one's lines
whi:            .res 10
tn:             .res 1
gk:             .res 1
th:             .res 1
ml:             .res 1
mh:             .res 1

        .segment "LOWDATA"

; Line addresses of HGR page 1.
_hgr_rowlo:
        .repeat 192, I
        .byte   <($2000 + (I & 7) * $400 + ((I >> 3) & 7) * $80 + (I >> 6) * $28)
        .endrep
_hgr_rowhi:
        .repeat 192, I
        .byte   >($2000 + (I & 7) * $400 + ((I >> 3) & 7) * $80 + (I >> 6) * $28)
        .endrep

; Colour pixel c (0..139) covers mono pixels 2c and 2c+1: their bytes and
; bit masks.
_hgr_cxb0:
cxb0:
        .repeat 140, I
        .byte   (2 * I) / 7
        .endrep
_hgr_cxb1:
cxb1:
        .repeat 140, I
        .byte   (2 * I + 1) / 7
        .endrep
cxm0:
        .repeat 140, I
        .byte   1 << ((2 * I) .mod 7)
        .endrep
cxm1:
        .repeat 140, I
        .byte   1 << ((2 * I + 1) .mod 7)
        .endrep

; Glyph row (7 pixels) -> 14 pixels, each doubled: the two bytes.
dbl0:
        .repeat 128, I
        .byte   ((I & 1) * 3) | (((I >> 1) & 1) * 12) | (((I >> 2) & 1) * 48) | (((I >> 3) & 1) * 64)
        .endrep
dbl1:
        .repeat 128, I
        .byte   ((I >> 3) & 1) | (((I >> 4) & 1) * 6) | (((I >> 5) & 1) * 24) | (((I >> 6) & 1) * 96)
        .endrep

        .code

; ---------------------------------------------------------------------------
.proc   _hgr_init
        sta     $C000                   ; 80STORE off: RAMRD/RAMWRT (AUXMOVE)
                                        ; must reach aux $2000-$3FFF too
        bit     TXTCLR
        bit     MIXCLR
        bit     LOWSCR
        bit     HIRES
        lda     #0
        sta     _hg_y
        lda     #192
        sta     _hg_h
        ; fall through
.endproc

.proc   _hgr_clear
        lda     _hg_y
        sta     line
        lda     _hg_h
        beq     done
        sta     cnt
loop:   jsr     _frame_tick
        ldx     line
        lda     _hgr_rowlo,x
        sta     dst
        lda     _hgr_rowhi,x
        sta     dst+1
        lda     #0
        ldy     #39
:       sta     (dst),y
        dey
        bpl     :-
        inc     line
        dec     cnt
        bne     loop
done:   rts
.endproc

; set dst to line `line`, column hg_bx (and keep the clock going)
.proc   rowaddr
        jsr     _frame_tick
        ldx     line
        lda     _hgr_rowlo,x
        clc
        adc     _hg_bx
        sta     dst
        lda     _hgr_rowhi,x
        adc     #0
        sta     dst+1
        rts
.endproc

; ---------------------------------------------------------------------------
.proc   _hgr_fill
        lda     _hg_y
        sta     line
        lda     _hg_h
        beq     done
        sta     cnt
loop:   jsr     rowaddr
        ldy     #0
        ldx     _hg_bx
colp:   txa
        lsr     a
        lda     _hg_v0
        bcc     :+
        lda     _hg_v1
:       sta     (dst),y
        inx
        iny
        cpy     _hg_n
        bne     colp
        inc     line
        dec     cnt
        bne     loop
done:   rts
.endproc

; ---------------------------------------------------------------------------
.proc   _hgr_glyph
        lda     _hg_y
        sta     line
        lda     _hg_bx
        lsr     a
        lda     _hg_mask
        bcc     :+
        lda     _hg_mask1
:       sta     col                     ; the mask for this column
        ldy     #0
loop:   sty     cnt
        jsr     rowaddr
        ldy     cnt
        lda     (_hg_src),y
        and     col
        ora     _hg_hi
        ldy     #0
        sta     (dst),y
        inc     line
        ldy     cnt
        iny
        cpy     #8
        bne     loop
        rts
.endproc

; ---------------------------------------------------------------------------
.proc   _hgr_glyph2
        lda     _hg_y
        sta     line
        ldy     #0
loop:   sty     cnt
        jsr     rowaddr
        ldy     cnt
        lda     (_hg_src),y
        tax
        lda     dbl0,x
        and     _hg_mask
        ora     _hg_hi
        ldy     #0
        sta     (dst),y
        lda     dbl1,x
        and     _hg_mask1
        ora     _hg_hi
        iny
        sta     (dst),y
        inc     line
        ldy     cnt
        iny
        cpy     #8
        bne     loop
        rts
.endproc

; ---------------------------------------------------------------------------
; hgr_blit / hgr_save: hg_h lines of hg_n bytes; hg_src advances as it goes.
.proc   _hgr_blit
        lda     #0
        beq     rect
.endproc

.proc   _hgr_save
        lda     #1
        ; fall through
.endproc

.proc   rect
        sta     col                     ; 0 to the screen, 1 from it
        lda     _hg_y
        sta     line
        lda     _hg_h
        beq     done
        sta     cnt
loop:   jsr     rowaddr
        ldy     #0
        lda     col
        bne     save
:       lda     (_hg_src),y
        sta     (dst),y
        iny
        cpy     _hg_n
        bne     :-
        beq     next
save:   lda     (dst),y
        sta     (_hg_src),y
        iny
        cpy     _hg_n
        bne     save
next:   lda     _hg_src                 ; hg_src += hg_n
        clc
        adc     _hg_n
        sta     _hg_src
        bcc     :+
        inc     _hg_src+1
:       inc     line
        dec     cnt
        bne     loop
done:   rts
.endproc

; ---------------------------------------------------------------------------
; Sprite rows, a lit pixel at a time: the colour's pixel operation is chosen
; once (setcol), and a row all on the screen goes 8 pixels unrolled with no
; checks. (Tried: whole screen bytes through nibble tables; these pictures
; are too sparse for it to win.)

; (in low memory, below the screen: the core has no room)
        .segment "LOWCODE"
; setcol: the colour in A (0..5) for drawrow's pixels.
.proc   setcol
        tax
        lda     optlo,x
        sta     pixop+1
        lda     opthi,x
        sta     pixop+2
        rts
optlo:  .byte   <op_black, <op_white, <op_violet, <op_green, <op_blue, <op_orange
opthi:  .byte   >op_black, >op_white, >op_violet, >op_green, >op_blue, >op_orange
.endproc

pixop:  jmp     $FFFF                   ; (setcol's choice; X = colour pixel)

; X = colour pixel. Each op updates one or two screen bytes; X is kept.
.proc   op_white
        ldy     cxb0,x
        lda     (dst),y
        ora     cxm0,x
        sta     (dst),y
        ldy     cxb1,x
        lda     (dst),y
        ora     cxm1,x
        sta     (dst),y
        rts
.endproc

.proc   op_black
        ldy     cxb0,x
        lda     cxm0,x
        eor     #$FF
        and     (dst),y
        sta     (dst),y
        ldy     cxb1,x
        lda     cxm1,x
        eor     #$FF
        and     (dst),y
        sta     (dst),y
        rts
.endproc

.proc   op_violet
        ldy     cxb0,x
        lda     (dst),y
        ora     cxm0,x
        and     #$7F
        sta     (dst),y
        rts
.endproc

.proc   op_blue
        ldy     cxb0,x
        lda     (dst),y
        ora     cxm0,x
        ora     #$80
        sta     (dst),y
        rts
.endproc

.proc   op_green
        ldy     cxb1,x
        lda     (dst),y
        ora     cxm1,x
        and     #$7F
        sta     (dst),y
        rts
.endproc

.proc   op_orange
        ldy     cxb1,x
        lda     (dst),y
        ora     cxm1,x
        ora     #$80
        sta     (dst),y
        rts
.endproc

        .code

; drawrow: hg_wb bytes from hg_src, their first pixel at colour pixel
; curx - 64, into dst; fastf: all of them on the screen (no checks).
.macro  BIT1
        asl     bits
        bcc     :+
        jsr     pixop
:       inx
        .endmacro

.proc   drawrow
        ldy     #0
        lda     fastf
        beq     slow
        lda     curx
        sec
        sbc     #64
        tax
fb:     lda     (_hg_src),y
        beq     fz
        sta     bits
        sty     bytes
        BIT1
        BIT1
        BIT1
        BIT1
        BIT1
        BIT1
        BIT1
        BIT1
        ldy     bytes
        iny
        cpy     _hg_wb
        bcc     fb
        rts
fz:     txa                             ; empty: 8 pixels on
        clc
        adc     #8
        tax
        iny
        cpy     _hg_wb
        bcc     fb
        rts
slow:   sty     bytes                   ; part off the screen: each pixel checked
        lda     curx
        sta     pxc
sbyte:  ldy     bytes
        lda     (_hg_src),y
        bne     sbits
        lda     pxc
        clc
        adc     #8
        sta     pxc
        jmp     snext
sbits:  sta     bits
        lda     #8
        sta     col
sbit:   asl     bits
        bcc     sskip
        lda     pxc
        sec
        sbc     #64
        bcc     sskip                   ; left of the screen
        cmp     #140
        bcs     sskip                   ; right of it
        tax
        jsr     pixop
sskip:  inc     pxc
        dec     col
        bne     sbit
snext:  inc     bytes
        lda     bytes
        cmp     _hg_wb
        bne     sbyte
        rts
.endproc

; ---------------------------------------------------------------------------
.proc   _hgr_spr
        lda     _hg_col
        jsr     setcol
        lda     _hg_y
        sta     line
        lda     _hg_h
        bne     :+
        rts
:       sta     cnt
        lda     _hg_x
        sta     curx
        lda     #0                      ; (checked: these are seldom drawn)
        sta     fastf
rows:   jsr     _frame_tick
        ldx     line
        lda     _hgr_rowlo,x
        sta     dst
        lda     _hgr_rowhi,x
        sta     dst+1
        jsr     drawrow
        lda     _hg_src                 ; next source row
        clc
        adc     _hg_wb
        sta     _hg_src
        bcc     :+
        inc     _hg_src+1
:       inc     line
        dec     cnt
        bne     rows
        rts
.endproc

; ---------------------------------------------------------------------------
; void spr_lines(void): lines sl_y0 .. sl_y1 - 1, each at once for the
; sl_nc slots in sl_cl: (sl_pass 0 or 2) the old rows back, the last drawn
; first, unless spr_bg put one back already (hit bit 1, the line in
; lift_y0 .. lift_y1 - 1); then (1 or 2) the new rows saved and drawn, in
; order. The rings' offsets and the pictures' rows move on in gfx.c's d_*.
SVP     = _pl_own2                      ; (hgr_plot's: not in use)
AUXP    = _pl_own3
DP      = _pl_white

.proc   _spr_lines
        lda     #$FF
        sta     lastcol
        ldy     _sl_nc                  ; each one's lines, old and new: a line
win:    dey                             ; outside them costs two compares
        bmi     wdone
        ldx     _sl_cl,y
        lda     #255
        sta     wlo,y
        lda     #0
        sta     whi,y
        lda     _s_drawn,x
        beq     :+
        lda     _s_sy,x
        sta     wlo,y
        clc
        adc     _s_sh,x
        sta     whi,y
:       lda     _d_show,x
        beq     win
        lda     _d_sy,x
        cmp     wlo,y
        bcs     :+
        sta     wlo,y
:       clc
        adc     _d_sh,x
        cmp     whi,y
        bcc     win
        sta     whi,y
        bcs     win
wdone:  lda     _sl_y0
        sta     line
lp:     lda     line
        cmp     _sl_y1
        bcc     go
        rts
go:     tax
        lda     _hgr_rowlo,x
        sta     dst
        lda     _hgr_rowhi,x
        sta     dst+1
        lda     _sl_pass
        cmp     #1
        bne     :+
        jmp     draws
:       lda     _sl_nc
        sta     kk
put:    dec     kk
        bmi     draws
        ldy     kk
        lda     line
        cmp     wlo,y
        bcc     put
        cmp     whi,y
        bcs     put
        ldx     _sl_cl,y
        lda     _s_drawn,x
        beq     put
        lda     line
        sec
        sbc     _s_sy,x
        cmp     _s_sh,x
        bcs     put
        lda     _hit,x
        and     #2
        beq     pdo
        lda     line
        cmp     _lift_y0
        bcc     pdo
        cmp     _lift_y1
        bcc     padv
pdo:    txa                             ; save_at[x] + d_oo[x] -> dst + s_bx[x]
        asl     a
        tay
        lda     _save_at,y
        clc
        adc     _d_oo,x
        sta     SVP
        lda     _save_at+1,y
        adc     #0
        sta     SVP+1
        lda     dst
        clc
        adc     _s_bx,x
        sta     DP
        lda     dst+1
        adc     #0
        sta     DP+1
        ldy     _s_n,x
        dey
:       lda     (SVP),y
        sta     (DP),y
        dey
        bpl     :-
padv:   lda     _d_oo,x
        clc
        adc     _s_st,x
        cmp     _d_oend,x
        bcc     :+
        lda     #0
:       sta     _d_oo,x
        jmp     put
draws:  lda     _sl_pass
        bne     :+
        jmp     next
:       lda     #0
        sta     kk
drw:    ldy     kk
        cpy     _sl_nc
        bcc     :+
        jmp     next
:       lda     line
        cmp     wlo,y
        bcc     dskip
        cmp     whi,y
        bcs     dskip
        ldx     _sl_cl,y
        lda     _d_show,x
        beq     dskip
        lda     line
        sec
        sbc     _d_sy,x
        cmp     _d_sh,x
        bcc     :+
dskip:  jmp     dnext
:       stx     slot
        txa                             ; dst + d_bx[x] -> save_at[x] + d_no[x]
        asl     a
        tay
        lda     _save_at,y
        clc
        adc     _d_no,x
        sta     SVP
        lda     _save_at+1,y
        adc     #0
        sta     SVP+1
        lda     dst
        clc
        adc     _d_bx,x
        sta     DP
        lda     dst+1
        adc     #0
        sta     DP+1
        ldy     _d_n,x
        dey
:       lda     (DP),y
        sta     (SVP),y
        dey
        bpl     :-
        lda     _d_no,x
        clc
        adc     _d_st,x
        cmp     _d_nend,x
        bcc     :+
        lda     #0
:       sta     _d_no,x
        txa                             ; the picture's row: d_src[x] (aux)
        asl     a
        tay
        lda     _d_src,y
        sta     AUXP
        clc
        adc     _d_wb,x
        sta     _d_src,y
        lda     _d_src+1,y
        sta     AUXP+1
        adc     #0
        sta     _d_src+1,y
        lda     _d_wb,x
        sta     _hg_wb
        tay
        dey
        jsr     auxrow
        ldx     slot
        lda     _d_col,x
        cmp     lastcol
        beq     :+
        sta     lastcol
        jsr     setcol
        ldx     slot
:       lda     _d_cx,x
        sta     curx
        lda     _d_fast,x
        sta     fastf
        lda     #<fbuf
        sta     _hg_src
        lda     #>fbuf
        sta     _hg_src+1
        jsr     drawrow
dnext:  inc     kk
        jmp     drw
next:   jsr     _frame_tick
        inc     line
        jmp     lp
.endproc

; ---------------------------------------------------------------------------
; void spr_one(void): spr_lines for one sprite (slot sl_cl[0]) that shares its
; lines with no other: its values once, its rings and picture as running
; offsets and pointers. A line put back is not saved again when the sprite
; keeps its bytes (it would save what it just put back).
.proc   _spr_one
        ldx     _sl_cl
        txa
        asl     a
        tay
        lda     _save_at,y
        sta     sbase
        lda     _save_at+1,y
        sta     sbase+1
        lda     _s_drawn,x
        sta     o_on
        lda     _s_sy,x
        sta     o_y0
        clc
        adc     _s_sh,x
        sta     o_y1
        lda     _s_bx,x
        sta     o_bx
        lda     _s_n,x
        sta     o_n
        lda     _s_st,x
        sta     o_st
        lda     _d_oend,x
        sta     o_end
        lda     _d_oo,x
        sta     oo
        lda     _hit,x
        and     #2
        sta     o_lift
        lda     _d_show,x
        sta     n_on
        lda     _d_sy,x
        sta     n_y0
        clc
        adc     _d_sh,x
        sta     n_y1
        lda     _d_bx,x
        sta     n_bx
        lda     _d_n,x
        sta     n_n
        lda     _d_st,x
        sta     n_st
        lda     _d_nend,x
        sta     n_end
        lda     _d_no,x
        sta     no
        lda     _d_wb,x
        sta     _hg_wb
        sec
        sbc     #1
        sta     wbm1
        lda     _d_cx,x
        sta     cxs
        lda     _d_fast,x
        sta     fsts
        lda     _d_src,y
        sta     AUXP
        lda     _d_src+1,y
        sta     AUXP+1
        lda     #0                      ; same bytes, same ring: no saving again
        sta     same
        lda     o_on
        and     n_on
        beq     :+
        lda     o_bx
        cmp     n_bx
        bne     :+
        lda     o_n
        cmp     n_n
        bne     :+
        lda     o_st
        cmp     n_st
        bne     :+
        inc     same
:       lda     _d_col,x
        jsr     setcol
        lda     _sl_pass                ; the rows it draws, fetched at once
        beq     nofetch
        lda     n_on
        beq     nofetch
        lda     n_y1                    ; lines max(n_y0, y0) .. min(n_y1, y1) - 1
        cmp     _sl_y1
        bcc     :+
        lda     _sl_y1
:       sta     tl
        lda     n_y0
        cmp     _sl_y0
        bcs     :+
        lda     _sl_y0
:       sta     th
        lda     tl
        sec
        sbc     th
        bcc     nofetch
        beq     nofetch
        ldx     _hg_wb
        jsr     mul8
        tay
        dey
        jsr     auxrow
nofetch:
        lda     #<fbuf
        sta     _hg_src
        lda     #>fbuf
        sta     _hg_src+1
        lda     _sl_y0
        sta     line
lp:     lda     line
        cmp     _sl_y1
        bcc     go
        rts
go:     tax
        lda     _hgr_rowlo,x
        sta     dst
        lda     _hgr_rowhi,x
        sta     dst+1
        lda     #$FF
        sta     putl
        lda     o_on                    ; the old row
        beq     new
        lda     line
        cmp     o_y0
        bcc     new
        cmp     o_y1
        bcs     new
        lda     _sl_pass
        cmp     #1
        beq     new
        lda     o_lift
        beq     odo
        lda     line
        cmp     _lift_y0
        bcc     odo
        cmp     _lift_y1
        bcc     oadv
odo:    lda     sbase
        clc
        adc     oo
        sta     SVP
        lda     sbase+1
        adc     #0
        sta     SVP+1
        lda     dst
        clc
        adc     o_bx
        sta     DP
        lda     dst+1
        adc     #0
        sta     DP+1
        ldy     o_n
        dey
:       lda     (SVP),y
        sta     (DP),y
        dey
        bpl     :-
        lda     #0
        sta     putl
oadv:   lda     oo
        clc
        adc     o_st
        cmp     o_end
        bcc     :+
        lda     #0
:       sta     oo
new:    lda     _sl_pass                ; the new row
        beq     next
        lda     n_on
        beq     next
        lda     line
        cmp     n_y0
        bcc     next
        cmp     n_y1
        bcs     next
        lda     same
        beq     nsave
        lda     putl
        beq     nsaved
nsave:  lda     sbase
        clc
        adc     no
        sta     SVP
        lda     sbase+1
        adc     #0
        sta     SVP+1
        lda     dst
        clc
        adc     n_bx
        sta     DP
        lda     dst+1
        adc     #0
        sta     DP+1
        ldy     n_n
        dey
:       lda     (DP),y
        sta     (SVP),y
        dey
        bpl     :-
nsaved: lda     no
        clc
        adc     n_st
        cmp     n_end
        bcc     :+
        lda     #0
:       sta     no
        lda     cxs
        sta     curx
        lda     fsts
        sta     fastf
        jsr     drawrow
        lda     _hg_src                 ; the next row
        clc
        adc     _hg_wb
        sta     _hg_src
        bcc     next
        inc     _hg_src+1
next:   jsr     _frame_tick             ; (a line of a wide sprite can take a
        inc     line                    ; good part of the blank)
        jmp     lp
.endproc

; ---------------------------------------------------------------------------
; mul8: A * X -> A (low), X (high)
.proc   mul8
        sta     mcand
        stx     mplier
        lda     #0
        sta     mlo
        ldx     #8
loop:   lsr     mplier
        bcc     :+
        clc
        adc     mcand
:       ror     a
        ror     mlo
        dex
        bne     loop
        tax
        lda     mlo
        rts
.endproc

; void src_of(uint8_t i): where slot i would be drawn now, into gfx.c's d_*
; (d_show 0: nowhere). Its shape (hgr_shapes, 8 bytes each), frame and
; mirroring give the picture's rows in aux RAM; y clips at the top and the
; bottom; x (colour clocks, -64..210 kept) becomes colour pixel cx - 64 at
; 7/8, its bytes and phase; a save ring's row (st) and rows from the shape.
SH       = _hgr_shapes
.proc   _src_of
        pha
        jsr     _frame_tick             ; (the planning keeps the clock too)
        pla
        tax
        lda     #0
        sta     _d_show,x
        lda     _s_on,x
        bne     :+
        rts
:       stx     slot
        lda     _s_shape,x
        asl     a
        asl     a
        asl     a
        tay
        sty     shp
        lda     _s_frame,x
        sta     frm
        lda     _s_flags,x
        and     #1                      ; SPR_FLIPX: the mirrored frames
        beq     nofl
        lda     _s_shape,x
        cmp     #2                      ; SH_MULE: 4 on
        bne     :+
        lda     frm
        adc     #3                      ; (carry set: + 4)
        sta     frm
        bne     nofl
:       cmp     #1                      ; SH_FIGURE: a species' first two, 2 on
        bne     nofl
        lda     frm
:       cmp     #10
        bcc     :+
        sbc     #10
        bcs     :-
:       cmp     #2
        bcs     nofl
        lda     frm
        adc     #2                      ; (carry clear)
        sta     frm
nofl:   lda     SH+5,y                  ; src = data + frame * sz
        ldx     frm
        jsr     mul8
        ldy     shp
        clc
        adc     SH,y
        sta     srcl
        txa
        adc     SH+1,y
        sta     srch
        ldx     slot                    ; y: off the top, its rows skipped
        lda     SH+4,y
        sta     hh
        lda     _s_yh,x
        beq     ypos
        cmp     #$FF
        bne     hide
        lda     _s_yl,x                 ; -yy lines off the top
        eor     #$FF
        clc
        adc     #1
        cmp     hh
        bcs     hide
        sta     yy
        lda     hh
        sec
        sbc     yy
        sta     hh
        lda     yy
        ldx     SH+2,y                  ; src += skip * wb
        jsr     mul8
        clc
        adc     srcl
        sta     srcl
        txa
        adc     srch
        sta     srch
        ldy     shp
        ldx     slot
        lda     #0
        beq     yset
hide:   rts
ypos:   lda     _s_yl,x
        cmp     #192
        bcs     hide
yset:   sta     yy
        clc                             ; off the bottom
        adc     hh
        bcs     :+
        cmp     #193
        bcc     yok
:       lda     #192
        sec
        sbc     yy
        sta     hh
yok:    lda     _s_xl,x                 ; t = x + 64, kept to 0..274
        clc
        adc     #64
        sta     tl
        lda     _s_xh,x
        adc     #0
        sta     th
        bmi     tzero                   ; left of -64
        beq     tok
        cmp     #1
        bne     tmax
        lda     tl
        cmp     #<274
        bcc     tok
tmax:   lda     #<274                   ; right of 210
        sta     tl
        lda     #>274
        sta     th
        bne     tok
tzero:  lda     #0
        sta     tl
        sta     th
tok:    lda     tl                      ; cx = (t * 8 - t) / 8 + 8
        sta     ml
        lda     th
        sta     mh
        asl     ml
        rol     mh
        asl     ml
        rol     mh
        asl     ml
        rol     mh
        lda     ml
        sec
        sbc     tl
        sta     ml
        lda     mh
        sbc     th
        lsr     a
        ror     ml
        lsr     a
        ror     ml
        lsr     a
        ror     ml
        lda     ml
        clc
        adc     #8
        sta     cxv
        cmp     #204                    ; left > 139: off the right
        bcs     hide2
        clc
        adc     SH+3,y                  ; right < 0: off the left
        bcs     :+
        cmp     #65
        bcs     :+
hide2:  rts
:       lda     cxv                     ; right = cx - 64 + w - 1, at most 139
        sec
        sbc     #65
        clc
        adc     SH+3,y
        cmp     #140
        bcc     :+
        lda     #139
:       sta     rightc
        ldx     slot
        lda     cxv
        sta     _d_cx,x
        ldy     #0                      ; leftc: left, at least 0
        sec
        sbc     #64
        bcc     :+
        tay
:       sty     leftc
        lda     #0                      ; all of its pixels on the screen?
        sta     _d_fast,x
        lda     cxv
        cmp     #64
        bcc     :+
        ldy     shp
        lda     SH+2,y                  ; cx + wb * 8 <= 204
        asl     a
        asl     a
        asl     a
        clc
        adc     cxv
        bcs     :+
        cmp     #205
        bcs     :+
        inc     _d_fast,x
:
        ldy     leftc
        lda     cxb0,y
        sta     _d_bx,x
        ldy     rightc
        lda     _hgr_cxb1,y
        sec
        sbc     _d_bx,x
        clc
        adc     #1
        sta     _d_n,x
        ldy     shp
        lda     SH+6,y                  ; st, rows, the ring's end
        sta     _d_st,x
        lda     SH+7,y
        sta     _d_rows,x
        cmp     hh                      ; no taller than the ring
        bcs     :+
        sta     hh
:       lda     SH+6,y
        ldx     SH+7,y
        jsr     mul8
        ldx     slot
        sta     _d_nend,x
        lda     yy                      ; its first line's row: yy % rows * st
        ldy     shp
:       cmp     SH+7,y
        bcc     :+
        sbc     SH+7,y
        bcs     :-
:       ldx     SH+6,y
        jsr     mul8
        ldx     slot
        sta     _d_no,x
        sta     _d_no0,x
        lda     yy
        sta     _d_sy,x
        lda     hh
        sta     _d_sh,x
        lda     SH+2,y
        sta     _d_wb,x
        lda     _s_col,x
        tay
        lda     _hcol,y
        sta     _d_col,x
        txa
        asl     a
        tay
        lda     srcl
        sta     _d_src,y
        lda     srch
        sta     _d_src+1,y
        lda     #1
        sta     _d_show,x
        rts
.endproc

; ---------------------------------------------------------------------------
; void spr_groups(void): the flush's sprites (g_cl, g_nc), lines g_y0 ..
; g_y1 - 1, pass g_pass (0: put back, then draw; 1 draw; 2 both; 4 only
; put back, for spr_bg): those whose rectangles (old and new) meet, through
; others too, are done together, a line at a time on their own lines
; (spr_lines); one that meets none alone (spr_one).
.proc   _spr_groups
        ldy     #0                      ; the rectangles
rect:   cpy     _g_nc
        bcc     :+
        jmp     sort
:       ldx     _g_cl,y
        lda     #255
        sta     glo,y
        sta     gxl,y
        lda     #0
        sta     ghi,y
        sta     gxh,y
        lda     _d_show,x
        beq     old
        lda     _d_sy,x
        sta     glo,y
        clc
        adc     _d_sh,x
        sta     ghi,y
        lda     _d_bx,x
        sta     gxl,y
        clc
        adc     _d_n,x
        sta     gxh,y
old:    lda     _s_drawn,x
        beq     clip
        lda     _s_sy,x
        cmp     glo,y
        bcs     :+
        sta     glo,y
:       clc
        adc     _s_sh,x
        cmp     ghi,y
        bcc     :+
        sta     ghi,y
:       lda     _s_bx,x
        cmp     gxl,y
        bcs     :+
        sta     gxl,y
:       clc
        adc     _s_n,x
        cmp     gxh,y
        bcc     clip
        sta     gxh,y
clip:   lda     glo,y                   ; within the flush's lines
        cmp     _g_y0
        bcs     :+
        lda     _g_y0
        sta     glo,y
:       lda     ghi,y
        cmp     _g_y1
        bcc     :+
        lda     _g_y1
        sta     ghi,y
:       iny
        jmp     rect
sort:   ldy     #0                      ; each its own group (glab: the least
:       tya                             ; of the group's places) ...
        sta     glab,y
        iny
        cpy     _g_nc
        bcc     :-
again:  jsr     _frame_tick
        lda     #0                      ; ... then any two that meet take the
        sta     tn                      ; lesser label, until none changes
        ldy     #0
gk1:    cpy     _g_nc
        bcs     gpass
        ldx     #0
gj1:    cpx     _g_nc
        bcs     gk1n
        lda     glab,x
        cmp     glab,y
        beq     gj1n                    ; already together
        lda     glo,x                   ; lo[x] < hi[y] && lo[y] < hi[x]
        cmp     ghi,y
        bcs     gj1n
        lda     glo,y
        cmp     ghi,x
        bcs     gj1n
        lda     gxl,x                   ; and the same for the bytes
        cmp     gxh,y
        bcs     gj1n
        lda     gxl,y
        cmp     gxh,x
        bcs     gj1n
        lda     glab,x                  ; meet: both the lesser label
        cmp     glab,y
        bcc     :+
        lda     glab,y
:       sta     glab,x
        sta     glab,y
        inc     tn
gj1n:   inx
        bne     gj1
gk1n:   iny
        bne     gk1
gpass:  lda     tn
        bne     again
        lda     #0                      ; each group, at its first place
        sta     gk
grp:    jsr     _frame_tick
        ldy     gk
        cpy     _g_nc
        bcc     :+
        rts
:       tya
        cmp     glab,y
        bne     gnext                   ; not a group's first
        ldx     #0                      ; its members, in order; their lines
        stx     tn
        lda     #255
        sta     _sl_y0
        lda     #0
        sta     _sl_y1
gm:     lda     glab,x
        cmp     gk
        bne     gmn
        lda     glo,x
        cmp     ghi,x
        bcs     gmn                     ; none of its lines here
        lda     _g_cl,x
        ldy     tn
        sta     _sl_cl,y
        inc     tn
        lda     glo,x
        cmp     _sl_y0
        bcs     :+
        sta     _sl_y0
:       lda     ghi,x
        cmp     _sl_y1
        bcc     gmn
        sta     _sl_y1
gmn:    inx
        cpx     _g_nc
        bcc     gm
        lda     tn
        beq     gnext
        sta     _sl_nc
        lda     _g_pass
        and     #3
        sta     _sl_pass
        lda     tn
        cmp     #1
        bne     glines
        jsr     _spr_one
        lda     _g_pass
        bne     gnext
        inc     _sl_pass
        jsr     _spr_one
        jmp     gnext
glines: jsr     _spr_lines
        lda     _g_pass
        bne     gnext
        inc     _sl_pass
        jsr     _spr_lines
gnext:  inc     gk
        jmp     grp
.endproc

; auxrow: Y+1 bytes of auxiliary RAM at AUXP to fbuf (main). It runs in
; the language card: with RAMRD set, main RAM's reads (code too) come from
; aux, the LC's don't. Interrupts off meanwhile.
        .segment "LC"
.proc   auxrow
        php
        sei
        sta     $C003                   ; RAMRD: aux
:       lda     (AUXP),y
        sta     fbuf,y                  ; (writes still go to main)
        dey
        bpl     :-
        sta     $C002                   ; RAMRD: main
        plp
        rts
.endproc
        .code

; ---------------------------------------------------------------------------
.proc   _hgr_plot
        lda     _hg_y
        sta     line
        lda     #32
        sta     cnt
row:    jsr     rowaddr
        ldy     #0                      ; the byte of the row
byte:   lda     (_pl_own1),y
        ora     (_pl_own2),y
        ora     (_pl_own3),y
        beq     noown
        and     _pl_om,y
        ora     _pl_hi
noown:  sta     ownv
        lda     (_pl_river),y
        beq     noriv
        and     rivmask,y
        ldx     ownv
        bne     noriv                   ; the owner's palette bit wins the byte
        ora     #$80                    ; blue
noriv:  ora     ownv
        ora     (_pl_white),y
        sta     (dst),y
        iny
        cpy     #4
        bne     byte
        ldx     #8                      ; the layers' next row
next:   lda     _pl_own1,x
        clc
        adc     #4
        sta     _pl_own1,x
        bcc     :+
        inc     _pl_own1+1,x
:       dex
        dex
        bpl     next
        inc     line
        dec     cnt
        bne     row
        rts
rivmask: .byte  $55, $2A, $55, $2A      ; the even colour pixels (blue) of each byte
.endproc
