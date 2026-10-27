;
; mapgfx.s -- gfx.c's map: plots in the original's map characters (mode 6),
; the mountains on player 2 one to a display row, the store on player 3.
;
; A map row's characters take their colour from bits 6-7 (PF0-3); each plot
; row (four display rows) has its own four colours, counted by the plots
; that hold them, so a fifth owner colour (white, for a plot on offer) can
; take a slot nobody in that row is using.
;
        .export _gfx_plot, _map_slot, _map_row_pf, _map_reset, _map_bar
        .export _mp_uses, _mbar, _mbar_col, _map_pr
        .import _row_pf, _row_hp, _row_cm, _row_sz, _row_lo, _row_hi
        .import _atari_col, _pf_dirty, _sp_dirty, _mtn_shape, _mtn_h, _store_p3
        .import incsp1
        .importzp c_sp, ptr1, ptr2, tmp1, tmp2, tmp3, tmp4

ROWS    = 24
ROW0    = 2                     ; the map's first display row
        .include "hw.inc"
P2      = PLAYERS + $0200
P3      = PLAYERS + $0300
C_BLUE  = 2

        .bss
mp_col:   .res 20               ; plot row * 4 + slot: the colour
_mp_uses: .res 20               ; ... and how many plots hold it
plot_slot: .res 45
plot_mtn: .res 45
_mbar:    .res ROWS             ; the time bar: lines lit, bit 7 the top
_mbar_col: .res 1
_map_pr:
pr:       .res 1                ; the plot's row, column
pc:       .res 1
pp:       .res 1
lc:       .res 4                ; its left and right characters
rc:       .res 4
band:     .res 1
cnt:      .res 1
kk:       .res 1

        .rodata
pick:   .byte   2, 0, 0,  1, 3, 0,  0, 2, 1     ; mountain shapes for 1, 2, 3
dx:     .byte   3, 7, 1, 5
pfoff:  .byte   0, ROWS, ROWS*2, ROWS*3
border_l: .byte $1D, $1E, $1E, $1F
border_r: .byte $20, $21, $21, $22

        .code

; void map_reset(void): the plot rows' colours are the players' again
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
        lda     #0
@p:     sta     plot_mtn,x
        lda     #$FF
        sta     plot_slot,x
        lda     #0
        dex
        bpl     @p
        ldx     #ROWS - 1
@b:     sta     _mbar,x
        dex
        bpl     @b
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

; void __fastcall__ map_bar(uint8_t row): the time bar's character in that
; row (map characters 1-8 are 8..1 lines, bottom aligned)
_map_bar:
        sta     tmp3
        sec
        sbc     #ROW0
        lsr     a
        lsr     a
        sta     pr
        ldx     tmp3
        lda     _mbar,x
        ldy     #0
@cnt:   asl     a
        bcc     @n
        iny
@n:     cmp     #0
        bne     @cnt
        tya
        beq     @put
        sta     cnt
        lda     _mbar_col
        jsr     _map_slot
        sta     kk
        lda     pr              ; the bar doesn't hold the colour
        asl     a
        asl     a
        clc
        adc     kk
        tay
        lda     _mp_uses,y
        sec
        sbc     #1
        sta     _mp_uses,y
        lda     kk
        lsr     a
        ror     a
        ror     a               ; slot << 6
        sta     kk
        lda     #9
        sec
        sbc     cnt
        ora     kk
@put:   pha
        lda     tmp3
        jsr     rowaddr
        pla
        ldy     #19
        sta     (ptr1),y
        rts

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
        ; the store: player 3
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

; The store on player 3: double width, black, over the plot's rows.
store:  lda     pr
        asl     a
        asl     a
        asl     a
        asl     a
        asl     a               ; 32 * row
        clc
        adc     #32 + 16        ; PMG byte of the plot's top
        sta     ptr1
        lda     #>P3
        sta     ptr1+1
        ldy     #31
@s:     tya
        lsr     a
        lsr     a
        tax
        lda     _store_p3,x
        sta     (ptr1),y
        dey
        bpl     @s
        ldy     #33             ; the river's first dots under it: player 3
        lda     #0              ; is still wide and black there (the DLI's
        sta     (ptr1),y        ; size change lands a line late)
        jsr     plotx
        sta     tmp1
        lda     pr
        asl     a
        asl     a
        clc
        adc     #ROW0 + ROWS*3
        tay
        ldx     #4
@r:     lda     tmp1
        sta     _row_hp,y
        lda     #0
        sta     _row_cm,y
        lda     #1
        sta     _row_sz,y
        iny
        dex
        bne     @r
        lda     #1
        sta     _sp_dirty
        rts

; A = hpos of plot (pr, pc)'s left edge: 48 + 8 + 16 * column
plotx:  lda     pc
        asl     a
        asl     a
        asl     a
        asl     a
        clc
        adc     #56
        rts

; The mountains of plot row pr, one to a display row, each plot's in the
; rows the others leave (as the original's: four to a plot row).
mountains:
        lda     pr
        asl     a
        asl     a
        asl     a
        asl     a
        asl     a
        clc
        adc     #32 + 16
        sta     ptr1
        lda     #>P2
        sta     ptr1+1
        lda     #0
        ldy     #31
@z:     sta     (ptr1),y
        dey
        bpl     @z
        lda     pr
        asl     a
        asl     a
        clc
        adc     #ROW0 + ROWS*2
        sta     tmp3            ; row_hp index of the first row
        tay
        lda     #0
        sta     _row_hp,y
        sta     _row_hp+1,y
        sta     _row_hp+2,y
        sta     _row_hp+3,y
        sta     band
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
        tax                     ; the shape
        lda     _mtn_h,x
        sta     tmp4
        ; copy its lines to the band's bottom: at band * 8 + 7 - h
        txa
        pha
        lda     band
        asl     a
        asl     a
        asl     a
        clc
        adc     #7
        sec
        sbc     tmp4
        tay
        pla
        tax                     ; shape: its bytes at x * 6 + 6 - h
        stx     tmp1
        txa
        asl     a
        adc     tmp1
        asl     a
        adc     #6
        sec
        sbc     tmp4
        tax
@cp:    lda     _mtn_shape,x
        sta     (ptr1),y
        inx
        iny
        dec     tmp4
        bne     @cp
        ; its row's position
        lda     band
        clc
        adc     pp
        and     #3
        tax
        jsr     plotx
        clc
        adc     dx,x
        pha
        lda     tmp3
        clc
        adc     band
        tay
        pla
        sta     _row_hp,y
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
@end:   lda     #1
        sta     _sp_dirty
        rts
