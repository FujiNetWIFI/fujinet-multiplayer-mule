;
; cells.s -- gfx.c's drawing rows: mode 4 cells made on the fly.
;
; A drawing row's screen byte is a character of the drawing charset (DYNCH,
; 0 = blank), taken from a pool when a cell gets its first pixel and given
; back when its last goes. A row has four colour slots (PF0-3); slot 3 is
; pixel value 3 of an inverse character.
;
        .export _cell_put, _row_slot, _dyn_reset, _dyn_release, _row_used, _row_lo, _row_hi
        .export _cp_r, _cp_cx, _cp_line, _cp_mask, _cp_col
        .import _row_pf, _atari_col, _pf_dirty, _row_mode
        .export _pfuse
        .importzp ptr1, ptr2, tmp1, tmp2

ROWS    = 24
DYNCH   = $A800
SCREEN  = $B400

        .segment "PMGFREE"      ; cleared by gfx_init
_cp_r:    .res 1
_cp_cx:   .res 1
_cp_line: .res 1
_cp_mask: .res 1
_cp_col:  .res 1                ; gfx.h colour (C_BLACK clears)
_row_used: .res ROWS            ; slots taken, bit per slot
dyn_free: .res 16               ; bit per character in use
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
        ; a free slot the rows above and below don't use for another
        ; colour (a colour change at a row's top lands late), else any
        ldx     #0
@nice:  ldy     _cp_r
        lda     _row_used,y
        and     bitv,x
        bne     @nn
        dey
        jsr     ok_in
        bne     @nn
        iny
        iny
        jsr     ok_in
        bne     @nn
        dey
        jmp     @take
@nn:    inx
        cpx     #4
        bcc     @nice
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

; Z set if row Y (may be -1 or 24: fine) leaves slot X alone or has colour
; tmp1 in it. Keeps X, Y.
ok_in:  cpy     #ROWS
        bcs     @yes
        tya
        pha
        lda     _row_mode,y
        tay
        lda     _pfuse,y
        bpl     @m
        pla
        pha
        tay
        lda     _row_used,y
@m:     and     bitv,x
        beq     @yes2
        pla
        pha
        clc
        adc     pfoff,x
        tay
        lda     _row_pf,y
        cmp     tmp1
        beq     @yes2
        pla
        tay
        lda     #1              ; Z clear: no
        rts
@yes2:  pla
        tay
@yes:   lda     #0
        rts

; void dyn_reset(void)
_dyn_reset:
        lda     #0
        ldx     #15
@c:     sta     dyn_free,x
        dex
        bpl     @c
        lda     #1              ; character 0 is the blank
        sta     dyn_free
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

; a free character in A (blanked), or 0
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
        pha
        jsr     glyph_a
        lda     #0
        ldy     #7
@z:     sta     (ptr2),y
        dey
        bpl     @z
        pla
        rts

; ptr2 = DYNCH + (A & $7F) * 8
glyph_a:
        and     #$7F
        ldx     #0
        stx     ptr2+1
        asl     a
        rol     ptr2+1
        asl     a
        rol     ptr2+1
        asl     a
        rol     ptr2+1
        sta     ptr2
        lda     ptr2+1
        clc
        adc     #>DYNCH
        sta     ptr2+1
        rts

; void cell_put(void): pixels cp_mask of line cp_line in cell cp_cx of row
; cp_r to colour cp_col
_cell_put:
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
        jsr     glyph_a
        ldy     _cp_line
        lda     _cp_mask
        eor     #$FF
        and     (ptr2),y
        sta     (ptr2),y
        ldy     #7
@chk:   lda     (ptr2),y
        bne     @rts
        dey
        bpl     @chk
        lda     ch
        jsr     _dyn_release
        lda     #0
        tay
        sta     (ptr1),y
@rts:   rts
@set:   jsr     _row_slot
        sta     slot
        lda     ch
        and     #$7F
        bne     @have
        jsr     alloc
        beq     @rts            ; none left
        sta     ch
@have:  ldx     slot
        cpx     #3
        bne     @ns
        lda     ch
        ora     #$80            ; inverse: pixel value 3 is PF3
        sta     ch
        lda     #$FF
        bne     @pat
@ns:    lda     pats,x
@pat:   and     _cp_mask
        sta     tmp1
        lda     ch
        ldy     #0
        sta     (ptr1),y
        jsr     glyph_a
        ldy     _cp_line
        lda     _cp_mask
        eor     #$FF
        and     (ptr2),y
        ora     tmp1
        sta     (ptr2),y
        rts
