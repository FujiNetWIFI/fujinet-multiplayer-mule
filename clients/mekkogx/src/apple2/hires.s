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
;                  pl_white as it is, pl_river in orange where the owner's
;                  layer leaves the byte free (point unused layers at zeros)
;   hgr_spr        a 1-bit sprite (hg_src, hg_wb bytes a row, hg_h lines)
;                  with its left colour pixel at hg_x - 64 (0..139 shows),
;                  line hg_y, in colour hg_col (0 black, 1 white, 2 violet,
;                  3 green, 4 blue, 5 orange)
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
curx:           .res 1
bits:           .res 1

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
cxm0:
        .repeat 140, I
        .byte   1 << ((2 * I) .mod 7)
        .endrep
_hgr_cxb1:
cxb1:
        .repeat 140, I
        .byte   (2 * I + 1) / 7
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
; hgr_spr: the pixel operation for the colour is chosen once (pixop).
.proc   _hgr_spr
        ldx     _hg_col
        lda     optlo,x
        sta     pixop+1
        lda     opthi,x
        sta     pixop+2
        lda     _hg_y
        sta     line
        lda     _hg_h
        bne     :+
        rts
:       sta     cnt
rows:   jsr     _frame_tick
        ldx     line
        lda     _hgr_rowlo,x
        sta     dst
        lda     _hgr_rowhi,x
        sta     dst+1
        lda     _hg_x
        sta     curx
        ldy     #0
        sty     bytes
rowbyte:
        ldy     bytes
        lda     (_hg_src),y
        bne     somebits
        lda     curx                    ; empty: skip 8 pixels
        clc
        adc     #8
        sta     curx
        jmp     nextbyte
somebits:
        sta     bits
        ldx     #8
bitloop:
        asl     bits
        bcc     skip
        lda     curx
        sec
        sbc     #64
        bcc     skip                    ; left of the screen
        cmp     #140
        bcs     skip                    ; right of it
        stx     col
        tax
pixop:  jsr     $FFFF
        ldx     col
skip:   inc     curx
        dex
        bne     bitloop
nextbyte:
        inc     bytes
        lda     bytes
        cmp     _hg_wb
        bne     rowbyte
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

optlo:  .byte   <op_black, <op_white, <op_violet, <op_green, <op_blue, <op_orange
opthi:  .byte   >op_black, >op_white, >op_violet, >op_green, >op_blue, >op_orange
.endproc

; X = colour pixel. Each op updates one or two screen bytes.
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
        ora     #$80                    ; orange
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
rivmask: .byte  $2A, $55, $2A, $55      ; the odd colour pixels (orange) of each byte
.endproc
