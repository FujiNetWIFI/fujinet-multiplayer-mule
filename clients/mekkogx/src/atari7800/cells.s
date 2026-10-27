;
; cells.s -- gfx.c's drawing rows: 160A cells made on the fly (the Atari
; port's src/atari/cells.s, with MARIA's character layout).
;
; A drawing row's screen byte is a cell of DYNB (0 = blank), taken from a
; pool when it gets its first pixel and given back when its last goes. A
; row has four colour slots (PF0-3, which maria.s/gfx.c make its zone's
; palettes); slot 3 is pixel value 3 of an inverse cell (bit 7), drawn in
; palette B. A cell's glyph line k is at DYNB + (7 - k) * 256 + c, and again
; at + c + 128, so the inverse code reads the same pixels.
;
        .include "a78.inc"
        .export _cell_put, _row_slot, _dyn_reset, _dyn_release, _row_used, _row_lo, _row_hi
        .export _cp_r, _cp_cx, _cp_line, _cp_mask, _cp_col, _cp_slot, _cell_puts
        .import _row_pf, _atari_col, _pf_dirty, _row_mode, _zdirty
        .export _pfuse
        .importzp ptr1, ptr2, tmp1, tmp2

        .bss
_cp_r:    .res 1
_cp_cx:   .res 1
_cp_line: .res 1
_cp_mask: .res 1
_cp_col:  .res 1                ; gfx.h colour (C_BLACK clears)
_cp_slot: .res 1                ; its slot (cell_puts)
_row_used: .res ROWS            ; slots taken, bit per slot
dyn_free: .res 16               ; bit per cell in use
ch:       .res 1
slot:     .res 1

        .rodata
_row_lo:
        .repeat ROWS, I
        .byte   <(SCREEN + I * 40)
        .endrep
_row_hi:
        .repeat ROWS, I
        .byte   >(SCREEN + I * 40)
        .endrep
bitv:   .byte   1, 2, 4, 8, 16, 32, 64, 128
pfoff:  .byte   0, ROWS, ROWS*2, ROWS*3
pats:   .byte   $55, $AA, $FF
; the colour registers (bit 0 = PF0) each row mode uses; $80: row_used.
; text: PF1 PF2; big, drawing: row_used; map, town: all
_pfuse: .byte   $06, $80, $80, $0F, $0F

        .code

; uint8_t __fastcall__ row_slot(uint8_t col): a colour slot in row cp_r
_row_slot:
        tax
        lda     _atari_col,x
        sta     tmp1
        ldx     #0
@find:  ldy     _cp_r
        lda     _row_used,y
        and     bitv,x
        beq     @n1
        tya
        clc
        adc     pfoff,x
        tay
        lda     _row_pf,y
        cmp     tmp1
        beq     @got
@n1:    inx
        cpx     #4
        bcc     @find
        ; a free slot (MARIA sets a row's colours before it: any will do)
        ldx     #0
@free:  ldy     _cp_r
        lda     _row_used,y
        and     bitv,x
        beq     @take
        inx
        cpx     #4
        bcc     @free
        ldx     #3              ; full: the last one
        bne     @got
@take:  lda     _row_used,y
        ora     bitv,x
        sta     _row_used,y
        tya
        clc
        adc     pfoff,x
        tay
        lda     tmp1
        sta     _row_pf,y
        lda     #1
        sta     _pf_dirty
@got:   txa
        ldx     #0
        rts

; void dyn_reset(void): every cell free, DYNB blank
_dyn_reset:
        lda     #0
        ldx     #15
@c:     sta     dyn_free,x
        dex
        bpl     @c
        lda     #1              ; cell 0 is the blank
        sta     dyn_free
        lda     #0
        tax
@z:     sta     DYNB,x
        sta     DYNB+$100,x
        sta     DYNB+$200,x
        sta     DYNB+$300,x
        sta     DYNB+$400,x
        sta     DYNB+$500,x
        sta     DYNB+$600,x
        sta     DYNB+$700,x
        inx
        bne     @z
        rts

; void __fastcall__ dyn_release(uint8_t c)
_dyn_release:
        and     #$7F
        beq     @r
        pha
        and     #7
        tax
        lda     bitv,x
        eor     #$FF
        sta     tmp2
        pla
        lsr     a
        lsr     a
        lsr     a
        tax
        lda     dyn_free,x
        and     tmp2
        sta     dyn_free,x
@r:     rts

; a free cell in A (blanked), or 0
alloc:  ldx     #0
@byte:  lda     dyn_free,x
        cmp     #$FF
        bne     @bit
        inx
        cpx     #16
        bcc     @byte
        lda     #0
        rts
@bit:   ldy     #0
@b:     lda     dyn_free,x
        and     bitv,y
        beq     @found
        iny
        bne     @b
@found: lda     dyn_free,x
        ora     bitv,y
        sta     dyn_free,x
        txa
        asl     a
        asl     a
        asl     a
        sta     tmp2
        tya
        ora     tmp2
        tax
        lda     #0
        sta     DYNB,x
        sta     DYNB+$100,x
        sta     DYNB+$200,x
        sta     DYNB+$300,x
        sta     DYNB+$400,x
        sta     DYNB+$500,x
        sta     DYNB+$600,x
        sta     DYNB+$700,x
        sta     DYNB+$80,x
        sta     DYNB+$180,x
        sta     DYNB+$280,x
        sta     DYNB+$380,x
        sta     DYNB+$480,x
        sta     DYNB+$580,x
        sta     DYNB+$680,x
        sta     DYNB+$780,x
        txa
        rts

; ptr2 = DYNB's page for line cp_line (lo 0)
linepg: lda     #7
        sec
        sbc     _cp_line
        clc
        adc     #>DYNB
        sta     ptr2+1
        lda     #0
        sta     ptr2
        rts

; void cell_put(void): pixels cp_mask of line cp_line in cell cp_cx of row
; cp_r to colour cp_col
_cell_put:
        lda     _cp_col
        beq     cell_at
        jsr     _row_slot
        sta     _cp_slot
; void cell_puts(void): the same, cp_col's slot in cp_slot already (a line of
; dots finds it once)
_cell_puts:
cell_at:
        ldx     _cp_r
        lda     _row_lo,x
        clc
        adc     _cp_cx
        sta     ptr1
        lda     _row_hi,x
        adc     #0
        sta     ptr1+1
        ldy     #0
        lda     (ptr1),y
        sta     ch
        lda     _cp_col
        bne     @set
        ; clear: give the cell back once it is empty
        lda     ch
        and     #$7F
        beq     @rts
        tay
        jsr     linepg
        lda     _cp_mask
        eor     #$FF
        and     (ptr2),y
        sta     (ptr2),y
        pha
        tya
        ora     #$80
        tay
        pla
        sta     (ptr2),y
        ; every line blank?
        tya
        and     #$7F
        tay
        lda     #>DYNB
        sta     ptr2+1
        ldx     #8
@chk:   lda     (ptr2),y
        bne     @rts
        inc     ptr2+1
        dex
        bne     @chk
        lda     ch
        jsr     _dyn_release
        lda     #0
        tay
        sta     (ptr1),y
        ldx     _cp_r
        lda     #1
        sta     _zdirty,x
@rts:   rts
@set:   lda     _cp_slot
        sta     slot
        lda     ch
        and     #$7F
        bne     @have
        jsr     alloc
        beq     @rts            ; none left
        sta     ch
        ldx     _cp_r
        lda     #1
        sta     _zdirty,x
@have:  ldx     slot
        cpx     #3
        bne     @ns
        lda     ch
        bmi     @inv
        ldx     _cp_r
        lda     #1
        sta     _zdirty,x
@inv:   lda     ch
        ora     #$80            ; inverse: pixel value 3 is PF3 (palette B)
        sta     ch
        lda     #$FF
        bne     @pat
@ns:    lda     pats,x
@pat:   and     _cp_mask
        sta     tmp1
        lda     ch
        ldy     #0
        sta     (ptr1),y
        and     #$7F
        tay
        jsr     linepg
        lda     _cp_mask
        eor     #$FF
        and     (ptr2),y
        ora     tmp1
        sta     (ptr2),y
        pha
        tya
        ora     #$80
        tay
        pla
        sta     (ptr2),y
        rts
