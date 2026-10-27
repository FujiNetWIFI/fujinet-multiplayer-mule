;
; maria.s -- the 7800's display: 24 zones of 8 lines, one to a row of the
; Atari port's screen (gfx.c), each with a display list of its own.
;
; A zone's list is the row's terrain (direct objects: the map's mountains,
; river and store, the time bar), its characters (indirect objects over the
; row's runs of SCREEN bytes) and the sprites' pieces in it (sprite.s), in
; that order: later objects are drawn over earlier ones, and the original
; puts the terrain behind the plots (PRIOR = 2). Each zone has two slots;
; the frame's DLI (nmi.s) points the DLL at the ones just built once a
; whole set is (dsp_ready), so a ship never tears. A slot keeps its
; background (terrain and characters) until the row changes (zdirty): then
; only its sprites are written again.
;
; The rows' modes (a78.inc RM_*) as MARIA draws them:
;   text  320A, 1-byte codes = the font's (TXTB), the row's text colour
;   big   160A, 2-byte codes (BIGB): bytes 20-39 hold them, made here from
;         the Atari's mode 6 bytes 0-19: slot 0/1 palette A value 1/2,
;         slot 2/3 palette B
;   draw  160A, 1-byte codes = the cells (DYNB); inverse ones palette B
;   map   160A, 2-byte codes (MTB), made here: slot 0 A1 or B2, 1 A2 or B3,
;         2 A3, 3 B1 (B = slot 3, 0, 1), one palette for a whole run; code
;         0 is blank (mapidx starts at 1)
;   town  160A, 1-byte codes = the town's characters (MTB), palette A
; An even zone's A and B are P0 and P1, an odd one's P2 and P3 (nmi.s);
; P4-P7 are the sprites' (colours 2 = 3) and the terrain's (colour 1).
;
        .include "a78.inc"
        .export _mt_init, _mt_compose, _mt_blank, _mt_dll, _mt_top, _mt_ntsc
        .export _zdirty, _zfront, _zflip, _dsp_ready
        .export _tr_lo, _tr_pw, _tr_x, _sp_n, _sp_lo, _sp_pw, _sp_x
        .export _mt_dma, _mt_shed, _mapidx, slot_lo, slot_hi
        .import _row_mode, _dsp_colbk, palfix
        .importzp tmp1, tmp2

ZSLOT    = 64           ; bytes in a zone's slot
TR_MAX   = 4            ; terrain objects a zone
SP_MAX   = 6            ; sprite pieces a zone
DMA_MAX  = 291          ; a zone's objects' DMA (MARIA cycles): with the
                        ; line's own 39 the DLI below lands in time (M0)
DMA_BASE = 16 + 6 + 17

        .segment "DLSLOTS"      ; $1800: zones 0-15, two slots each
slots_a: .res 16 * 2 * ZSLOT
        .segment "CONSBSS"      ; $2200: zones 16-23, then the DLL
slots_b: .res 8 * 2 * ZSLOT
_mt_dll: .res 3 * (4 + 24 + 4)
nulldl:  .res 2

        .segment "ZEROPAGE"
op:      .res 2                 ; the slot being written
rp:      .res 2                 ; the row
wi:      .res 1                 ; the write index in the slot
ci:      .res 1                 ; the row's cell

        .bss
_zdirty: .res ROWS              ; the row's background changed
zbgok:   .res ROWS              ; bit per slot: its background is current
zbglen:  .res ROWS * 2          ; ... ends here
zbgdma:  .res ROWS * 2          ; ... and costs this (DMA / 2)
_zfront: .res ROWS              ; the slot shown
_zflip:  .res ROWS              ; built: show the other from the next frame
_dsp_ready: .res 1              ; a set is built (nmi.s takes it)
_mt_top: .res 1                 ; DLL entries above the zones
_mt_ntsc: .res 1                ; 1 NTSC, 0 PAL
_tr_lo:  .res ROWS * TR_MAX
_tr_pw:  .res ROWS * TR_MAX
_tr_x:   .res ROWS * TR_MAX
_sp_n:   .res ROWS              ; sprite pieces: TXTB columns
_sp_lo:  .res ROWS * SP_MAX
_sp_pw:  .res ROWS * SP_MAX
_sp_x:   .res ROWS * SP_MAX
_mt_dma: .res ROWS              ; each zone's DMA / 2 as last built
_mt_shed: .res 1                ; sprite pieces left out (the harness)
_mapidx: .res 64                ; map glyph -> its index in MTB ($FF none)
zn:       .res 1
zs:      .res 1                 ; zn * 2 + the slot
bs:      .res 1                 ; the back slot
pa:      .res 1                 ; the zone's palette A << 5
pb:      .res 1                 ; and B
dma:     .res 2
rstart:  .res 1
rlen:    .res 1
rpal:    .res 1
gap:     .res 1
cw2:     .res 1
mode:    .res 1
cnt:     .res 1
kk:       .res 1

        .rodata
; zone zn's slot s at slot_lo/hi[zn * 2 + s]
slot_lo:
        .repeat 16, I
        .byte <(slots_a + I * 2 * ZSLOT), <(slots_a + I * 2 * ZSLOT + ZSLOT)
        .endrep
        .repeat 8, I
        .byte <(slots_b + I * 2 * ZSLOT), <(slots_b + I * 2 * ZSLOT + ZSLOT)
        .endrep
slot_hi:
        .repeat 16, I
        .byte >(slots_a + I * 2 * ZSLOT), >(slots_a + I * 2 * ZSLOT + ZSLOT)
        .endrep
        .repeat 8, I
        .byte >(slots_b + I * 2 * ZSLOT), >(slots_b + I * 2 * ZSLOT + ZSLOT)
        .endrep
row_lo:
        .repeat ROWS, I
        .byte <(SCREEN + I * 40)
        .endrep
row_hi:
        .repeat ROWS, I
        .byte >(SCREEN + I * 40)
        .endrep
sbit:   .byte   1, 2
; map: a slot's value in palette A, in B (0: not there)
vA:     .byte   1, 2, 3, 0
vB:     .byte   2, 3, 0, 1
vbase:  .byte   0, 37, 74       ; (value - 1) * 37: code 0 stays blank

        .code

; ---- start-up ---------------------------------------------------------------
; void mt_init(void): DMA off, empty slots, the DLL for this console (NTSC:
; 25 blank lines on top, PAL 50); DPPH/L point at it. nmi.s turns it on.
_mt_init:
        lda     #$7F
        sta     CTRL
        lda     #0
        sta     OFFSET
        sta     nulldl
        sta     nulldl+1
        sta     _dsp_ready
        ldx     #ROWS - 1
@z:     sta     _zdirty,x
        sta     zbgok,x
        sta     _zfront,x
        sta     _zflip,x
        sta     _sp_n,x
        dex
        bpl     @z
        ldx     #ROWS * TR_MAX - 1
@t:     sta     _tr_pw,x
        dex
        bpl     @t
        ldx     #2 * ROWS - 1   ; every slot ends at once
@s:     lda     slot_lo,x
        sta     op
        lda     slot_hi,x
        sta     op+1
        ldy     #0
        tya
        sta     (op),y
        iny
        sta     (op),y
        dex
        bpl     @s
        jsr     tvlines
        ldy     #0
        lda     _mt_ntsc
        beq     @pal
        lda     #15
        jsr     blank
        lda     #8
        jsr     blank
        lda     #2
        bne     @top
@pal:   lda     #15
        jsr     blank
        lda     #15
        jsr     blank
        lda     #15
        jsr     blank
        lda     #1
        jsr     blank
        lda     #4
@top:   sta     _mt_top
        ldx     #0
@zl:    lda     #7              ; 8 lines (the DLI bits are nmi.s's)
        sta     _mt_dll,y
        iny
        stx     tmp1
        txa
        asl     a
        tax
        lda     slot_hi,x
        sta     _mt_dll,y
        iny
        lda     slot_lo,x
        sta     _mt_dll,y
        iny
        ldx     tmp1
        inx
        cpx     #ROWS
        bcc     @zl
        lda     #$8F            ; the bottom: the frame's DLI, 16 lines
        jsr     blank
        lda     #15
        jsr     blank
        lda     #15
        jsr     blank
        lda     #15
        jsr     blank
        lda     #>_mt_dll
        sta     DPPH
        lda     #<_mt_dll
        sta     DPPL
        rts

blank:  sta     _mt_dll,y
        iny
        lda     #>nulldl
        sta     _mt_dll,y
        iny
        lda     #<nulldl
        sta     _mt_dll,y
        iny
        rts

; NTSC or PAL: MARIA's lines between two blanks (242 or 292)
tvlines:
@a:     bit     MSTAT
        bpl     @a
@b:     bit     MSTAT
        bmi     @b
        ldx     #0
        ldy     #0
@c:     sta     WSYNC
        inx
        bne     @d
        iny
@d:     bit     MSTAT
        bpl     @c
        lda     #1
        cpy     #0
        beq     @n
        cpx     #270 - 256
        bcc     @n
        lda     #0
@n:     sta     _mt_ntsc
        rts

; void mt_blank(void): a new screen: every zone shows nothing from now (a
; store of 0 to its shown list's second byte ends it at once) and the
; background is dsp_colbk, until the zones are built again and shown all
; together; their backgrounds are made again. (Not while a set waits for
; the frame's DLI: gfx.c waits.)
_mt_blank:
        ldx     #ROWS - 1
@z:     lda     #0
        sta     zbgok,x
        txa
        asl     a
        ora     _zfront,x
        tay
        lda     slot_lo,y
        sta     op
        lda     slot_hi,y
        sta     op+1
        ldy     #1
        lda     #0
        sta     (op),y
        dex
        bpl     @z
        lda     _dsp_colbk
        jsr     palfix
        sta     BACKGRND
        rts

; ---- building ---------------------------------------------------------------
; void mt_compose(void): every zone whose row or sprites changed (zdirty,
; zflip) gets its back slot written; the frame's DLI shows them all at
; once. Nothing while the last set waits for it.
_mt_compose:
        lda     _dsp_ready
        bne     @r
        sta     _mt_shed
        sta     zn
@zone:  ldx     zn
        lda     _zdirty,x
        beq     @nd
        lda     #0
        sta     _zdirty,x
        sta     zbgok,x
        lda     #1
        sta     _zflip,x
@nd:    lda     _zflip,x
        beq     @next
        jsr     build
@next:  inc     zn
        lda     zn
        cmp     #ROWS
        bcc     @zone
        lda     #1
        sta     _dsp_ready
@r:     rts

; zone zn's back slot: its background if that slot's is stale, then its
; sprites while the DMA allows, then the end
build:  ldx     zn
        lda     _zfront,x
        eor     #1
        sta     bs              ; the back slot
        txa
        asl     a
        ora     bs
        sta     zs
        tay
        lda     slot_lo,y
        sta     op
        lda     slot_hi,y
        sta     op+1
        txa                     ; even zones P0/P1, odd P2/P3
        and     #1
        beq     @p
        lda     #$40
@p:     sta     pa
        ora     #$20
        sta     pb
        ldy     bs
        lda     sbit,y
        and     zbgok,x
        bne     @have
        jsr     background
        ldy     zs
        lda     wi
        sta     zbglen,y
        lda     dma+1
        lsr     a
        lda     dma
        ror     a
        sta     zbgdma,y
        ldx     zn
        ldy     bs
        lda     sbit,y
        ora     zbgok,x
        sta     zbgok,x
        jmp     @spr
@have:  ldy     zs
        lda     zbglen,y
        sta     wi
        lda     #0
        sta     dma+1
        lda     zbgdma,y
        asl     a
        sta     dma
        rol     dma+1
@spr:   ldx     zn
        lda     _sp_n,x
        beq     @end
        sta     cnt
        txa                     ; zn * SP_MAX (< 144)
        asl     a
        sta     tmp1
        asl     a
        adc     tmp1
        tax
@pc:    lda     _sp_pw,x        ; 8 + 3 * width
        jsr     objdma
        clc
        adc     dma
        sta     tmp1
        lda     dma+1
        adc     #0
        cmp     #>DMA_MAX       ; past DMA_MAX: shed it
        bcc     @fits
        bne     @shed
        lda     tmp1
        cmp     #<DMA_MAX + 1
        bcs     @shed
        lda     #>DMA_MAX
@fits:  sta     dma+1
        lda     tmp1
        sta     dma
        ldy     wi
        cpy     #ZSLOT - 6
        bcs     @shed
        lda     _sp_lo,x
        sta     (op),y
        iny
        lda     _sp_pw,x
        sta     (op),y
        iny
        lda     #>TXTB
        sta     (op),y
        iny
        lda     _sp_x,x
        sta     (op),y
        iny
        sty     wi
        jmp     @np
@shed:  inc     _mt_shed
@np:    inx
        dec     cnt
        bne     @pc
@end:   ldy     wi
        lda     #0
        sta     (op),y
        iny
        sta     (op),y
        ldx     zn
        lda     dma+1
        lsr     a
        lda     dma
        ror     a
        sta     _mt_dma,x
        rts

; A = a 4-byte header's palette/width byte -> 8 + 3 * width in A
objdma: and     #$1F
        eor     #$1F
        clc
        adc     #1              ; the width
        sta     tmp2
        asl     a
        adc     tmp2
        adc     #8
        rts

; A to dma
adddma: clc
        adc     dma
        sta     dma
        bcc     @r
        inc     dma+1
@r:     rts

; zone zn's terrain and characters into the slot at op from its start; wi,
; dma
background:
        lda     #0
        sta     wi
        sta     dma+1
        lda     #DMA_BASE
        sta     dma
        lda     zn              ; its four (mapgfx.s TR_*: 0 = none)
        asl     a
        asl     a               ; zn * TR_MAX
        tax
        lda     #TR_MAX
        sta     cnt
@tr:    lda     _tr_pw,x
        beq     @trn
        ldy     wi
        lda     _tr_lo,x
        sta     (op),y
        iny
        lda     _tr_pw,x
        sta     (op),y
        iny
        lda     #>TXTB
        sta     (op),y
        iny
        lda     _tr_x,x
        sta     (op),y
        iny
        sty     wi
        lda     _tr_pw,x
        jsr     objdma
        jsr     adddma
@trn:   inx
        dec     cnt
        bne     @tr
@chars: ldx     zn
        lda     row_lo,x
        sta     rp
        lda     row_hi,x
        sta     rp+1
        lda     _row_mode,x
        sta     mode
        lda     #0
        sta     cw2
        lda     mode
        cmp     #RM_BIG
        beq     @two
        cmp     #RM_MAP
        beq     @two
        jmp     runs1
@two:   inc     cw2
        jmp     runs2

; ---- 1-byte codes: text, draw, town ----------------------------------------
; Runs of non-zero bytes of the row (two blanks end one: a header costs ten
; cycles, a blank code six), 32 at most; in a draw row the inverse cells
; (bit 7) are palette B and run apart.
runs1:  lda     #0
        sta     ci
@seek:  ldy     ci
@s:     cpy     #40
        bcs     @done
        lda     (rp),y
        bne     @start
        iny
        bne     @s
@start: sty     rstart
        jsr     pal1
        sta     rpal
        lda     #0
        sta     gap
        iny
@ext:   cpy     #40
        bcs     @emit
        tya
        sec
        sbc     rstart
        cmp     #32
        bcs     @emit
        lda     (rp),y
        bne     @nz
        iny
        inc     gap
        lda     gap
        cmp     #2
        bcc     @ext
        bcs     @emit
@nz:    jsr     pal1
        cmp     rpal
        bne     @emit
        lda     #0
        sta     gap
        iny
        bne     @ext
@emit:  sty     ci
        tya
        sec
        sbc     rstart
        sec
        sbc     gap
        sta     rlen
        lda     rstart
        asl     a
        asl     a               ; x = 4 * cell
        jsr     emit
        jmp     @seek
@done:  rts

; A = a 1-byte code -> its palette (pa, or pb for a draw row's inverse)
pal1:   ldx     mode
        cpx     #RM_DRAW
        bne     @a
        cmp     #$80
        bcc     @a
        lda     pb
        rts
@a:     lda     pa
        rts

; The indirect object for rlen codes at (rp) + rstart, at x = A, palette
; rpal; its DMA: 10 + 6 (or 9) a code.
emit:   sta     tmp1
        ldy     wi
        cpy     #ZSLOT - 5 - 2
        bcs     @full
        lda     rp
        clc
        adc     rstart
        sta     (op),y
        iny
        lda     #$60            ; indirect, write mode 0
        sta     (op),y
        iny
        lda     rp+1
        adc     #0
        sta     (op),y
        iny
        lda     #0
        sec
        sbc     rlen
        and     #$1F
        ora     rpal
        sta     (op),y
        iny
        lda     tmp1
        sta     (op),y
        iny
        sty     wi
        lda     rlen            ; 6 n (n <= 32: < 256)
        asl     a
        adc     rlen
        asl     a
        ldx     cw2
        beq     @add
        clc                     ; 9 n = 6 n + 3 n (n <= 20)
        adc     rlen
        adc     rlen
        adc     rlen
@add:   jsr     adddma
        lda     #10
        jmp     adddma
@full:  rts

; ---- 2-byte codes: big, map -------------------------------------------------
; Bytes 0-19 are the Atari's mode 6 characters (code = slot << 6 | glyph);
; the codes MARIA reads go into bytes 20-39, as the run's palette has them.
runs2:  lda     #0
        sta     ci
@seek:  ldy     ci
@s:     cpy     #20
        bcs     @done
        lda     (rp),y
        and     #$3F
        bne     @start
        jsr     put0
        iny
        bne     @s
@start: sty     rstart
        lda     (rp),y
        jsr     group
        sta     rpal
        lda     #0
        sta     gap
@ext:   cpy     #20
        bcs     @emit
        tya
        sec
        sbc     rstart
        cmp     #32
        bcs     @emit
        lda     (rp),y
        and     #$3F
        bne     @nz
        jsr     put0
        iny
        inc     gap
        lda     gap
        cmp     #2
        bcc     @ext
        bcs     @emit
@nz:    lda     (rp),y
        jsr     code2           ; in this run's palette? (C set: no)
        bcs     @emit
        jsr     put
        lda     #0
        sta     gap
        iny
        bne     @ext
@emit:  sty     ci
        tya
        sec
        sbc     rstart
        sec
        sbc     gap
        sta     rlen
        lda     rstart
        asl     a
        asl     a
        asl     a
        pha                     ; x = 8 * character
        lda     rstart
        clc
        adc     #20
        sta     rstart          ; (its codes)
        pla
        jsr     emit
        jmp     @seek
@done:  rts

; byte 20 + Y = A (Y kept)
put0:   lda     #0
put:    pha
        tya
        clc
        adc     #20
        tay
        pla
        sta     (rp),y
        tya
        sec
        sbc     #20
        tay
        rts

; A = an Atari code -> the palette its run takes (pa or pb): big slots 2-3
; and the map's slot 3 are B
group:  ldx     mode
        cpx     #RM_BIG
        bne     @map
        cmp     #$80
        bcc     @a
        lda     pb
        rts
@map:   cmp     #$C0
        bcc     @a
        lda     pb
        rts
@a:     lda     pa
        rts

; A = an Atari code (non-blank) -> carry clear and A its MARIA code if it
; can be drawn in palette rpal, else carry set. Y kept.
code2:  sta     tmp1
        lda     mode
        cmp     #RM_BIG
        bne     @map
        lda     tmp1            ; big: slot 0/1 A, 2/3 B; value 1 + (slot & 1)
        jsr     group
        cmp     rpal
        bne     @no
        lda     tmp1
        and     #$40
        asl     a               ; (slot & 1) << 7
        sta     tmp2
        lda     tmp1
        and     #$3F
        asl     a
        ora     tmp2
        clc
        rts
@no:    sec
        rts
@map:   lda     tmp1
        rol     a
        rol     a
        rol     a
        and     #3              ; the slot
        tax
        lda     rpal
        cmp     pa
        beq     @inA
        lda     vB,x
        jmp     @v
@inA:   lda     vA,x
@v:     beq     @no
        tax                     ; the value
        sty     kk
        lda     tmp1
        and     #$3F
        tay
        lda     _mapidx,y
        ldy     kk
        cmp     #$FF
        beq     @blank
        clc
        adc     vbase-1,x
        asl     a
        clc
        rts
@blank: lda     #0
        clc
        rts
