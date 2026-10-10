;
; gfxa.s -- gfx.c's inner loops (src/nes/gfx.c): a plot is 384 bytes of
; patterns made from two 96-byte masks, and in C a plot took four frames.
;
;   q_bytes     q_n bytes from (q_src) into the PPU queue at qh
;   plot_or     own |= (q_src), 96 bytes (a plot part, from the art's bank)
;   plot_row    one cell row of a plot (3 cells, 48 bytes) into cbuf: line
;               r of cell cx is own/wh[pr_k + 3 * r + cx]; a pixel is the
;               owner's (own) in colour pr_o0/pr_o1[cx] (the planes' masks),
;               else wh's in pr_w0/pr_w1, else 0
;
        .export         _q_bytes, _plot_or, _plot_row
        .exportzp       _q_src, _q_n
        .export         _pr_k, _pr_o0, _pr_o1, _pr_w0, _pr_w1
        .import         _ppuq, _qh, _own, _wh, _cbuf

.segment "ZEROPAGE"
_q_src:  .res 2
_q_n:    .res 1
cx:      .res 1
lines:   .res 1
mo0:     .res 1                 ; the cell's owner masks
mo1:     .res 1
to:      .res 1
tw:      .res 1
ta:      .res 1

.segment "BSS"
_pr_k:   .res 1
_pr_o0:  .res 3
_pr_o1:  .res 3
_pr_w0:  .res 1
_pr_w1:  .res 1

.segment "CODE"

_q_bytes:
        ldx     _qh
        ldy     #0
@l:     lda     (_q_src),y
        sta     _ppuq,x
        inx
        iny
        cpy     _q_n
        bne     @l
        stx     _qh
        rts

_plot_or:
        ldy     #95
@l:     lda     (_q_src),y
        ora     _own,y
        sta     _own,y
        dey
        bpl     @l
        rts

_plot_row:
        lda     #0
        sta     cx
@cell:  tax
        lda     _pr_o0,x
        sta     mo0
        lda     _pr_o1,x
        sta     mo1
        txa                     ; cell cx's 16 bytes: cbuf + 16 * cx
        asl     a
        asl     a
        asl     a
        asl     a
        tay
        txa
        clc
        adc     _pr_k
        tax
        lda     #8
        sta     lines
@line:  lda     _own,x
        sta     to
        eor     #$FF
        and     _wh,x
        sta     tw              ; wh's pixels the owner's don't cover
        and     _pr_w0
        sta     ta
        lda     to
        and     mo0
        ora     ta
        sta     _cbuf,y
        lda     tw
        and     _pr_w1
        sta     ta
        lda     to
        and     mo1
        ora     ta
        sta     _cbuf+8,y
        inx                     ; the next line: 3 bytes on
        inx
        inx
        iny
        dec     lines
        bne     @line
        inc     cx
        lda     cx
        cmp     #3
        bne     @cell
        rts
