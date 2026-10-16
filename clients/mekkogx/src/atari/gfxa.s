;
; gfxa.s -- the Atari display's interrupts (see atari.h, gfx.c).
;
; The display list is 24 blank lines and 24 rows of 8 scanlines; a DLI at
; the end of each line loads the next row's registers from the row tables,
; only the groups row_flags asks for (what differs from the row above), so
; a row can have its own charset, colours and player positions -- as the
; original's DLIs do (map $2D43, town $2EB9). The immediate VBI starts the
; tables over and runs the sound; it runs even while SIO is busy (CRITIC),
; so the screen and the music keep going during network calls.
;
; The players' rows are built in row_hp/cm/sz/flags (sprite.s, mapgfx.s,
; gfx.c) while the DLIs read their own copies, which the deferred VBI brings
; up to date once a set is whole (dsp_ready): built in place, the DLIs read
; rows half rewritten now and then (a ship torn in two). It runs after the
; OS's second stage has set the colours from their shadows (a longer
; immediate VBI let that land on row 1), a row at a time from the top, ahead
; of the beam.
;
        .export _dsp_init, _dli_on
        .export _row_flags, _row_chb, _row_pf, _row_hp, _row_cm, _row_sz
        .export _dsp_colbk, _dsp_prior, _dsp_sizem, _dsp_hposm, _dsp_ready
        .import _snd_vbi

ROWS    = 24

HPOSP0  = $D000
HPOSM0  = $D004
SIZEP0  = $D008
SIZEM   = $D00C
COLPM0  = $D012
COLPF0  = $D016
COLBK   = $D01A
PRIOR   = $D01B
GRACTL  = $D01D
DMACTL  = $D400
PMBASE  = $D407
CHBASE  = $D409
WSYNC   = $D40A
NMIEN   = $D40E
VDSLST  = $0200
SDMCTL  = $022F
SDLSTL  = $0230
GPRIOR  = $026F
COLOR0  = $02C4
COLOR4  = $02C8
PCOLR0  = $02C0
CHBAS   = $02F4
ATRACT  = $004D
SETVBV  = $E45C
XITVBV  = $E462
SYSVBV  = $E45F

        .segment "BSS"          ; the players' rows as built
_row_flags: .res ROWS
_row_hp:    .res ROWS * 4
_row_cm:    .res ROWS * 4
_row_sz:    .res ROWS * 4

        .segment "PMGFREE"      ; $B850-$BAFF: cleared by gfx_init
dl_flags:   .res ROWS           ; and as the DLIs read them (vbid)
_row_chb:   .res ROWS
_row_pf:    .res ROWS * 4
dl_hp:      .res ROWS * 4
dl_cm:      .res ROWS * 4
dl_sz:      .res ROWS * 4
_dsp_colbk: .res 1
_dsp_prior: .res 1
_dsp_sizem: .res 1
_dsp_hposm: .res 4
_dsp_ready: .res 1
_dli_on:    .res 1
dli_row:    .res 1
dli_f:      .res 1

        .code

        .rodata
pfrow0: .byte   0, ROWS, ROWS*2, ROWS*3

        .code

; void dsp_init(void): the interrupts (the display list is gfx.c's)
_dsp_init:
        lda     #0
        sta     NMIEN
        lda     #<dli
        sta     VDSLST
        lda     #>dli
        sta     VDSLST+1
        lda     #6              ; immediate VBI
        ldy     #<vbi
        ldx     #>vbi
        jsr     SETVBV
        lda     #7              ; deferred VBI
        ldy     #<vbid
        ldx     #>vbid
        jsr     SETVBV
        lda     #$C0
        sta     NMIEN
        rts

; A row's DLI: PF2 (a text row's background) goes in first thing after
; WSYNC; the rest follows as row_flags asks.
dli:    pha
        txa
        pha
        ldx     dli_row
        lda     _row_pf+ROWS*2,x
        sta     WSYNC
        sta     COLPF0+2
        lda     dl_flags,x
        beq     @done
        sta     dli_f
        lsr     dli_f
        bcc     @hp
        lda     _row_pf+ROWS,x
        sta     COLPF0+1
        lda     _row_chb,x
        sta     CHBASE
        lda     _row_pf,x
        sta     COLPF0
        lda     _row_pf+ROWS*3,x
        sta     COLPF0+3
@hp:    lsr     dli_f
        bcc     @cm
        lda     dl_hp,x
        sta     HPOSP0
        lda     dl_hp+ROWS,x
        sta     HPOSP0+1
        lda     dl_hp+ROWS*2,x
        sta     HPOSP0+2
        lda     dl_hp+ROWS*3,x
        sta     HPOSP0+3
@cm:    lsr     dli_f
        bcc     @sz
        lda     dl_cm,x
        sta     COLPM0
        lda     dl_cm+ROWS,x
        sta     COLPM0+1
        lda     dl_cm+ROWS*2,x
        sta     COLPM0+2
        lda     dl_cm+ROWS*3,x
        sta     COLPM0+3
@sz:    lsr     dli_f
        bcc     @done
        lda     dl_sz,x
        sta     SIZEP0
        lda     dl_sz+ROWS,x
        sta     SIZEP0+1
        lda     dl_sz+ROWS*2,x
        sta     SIZEP0+2
        lda     dl_sz+ROWS*3,x
        sta     SIZEP0+3
@done:  inc     dli_row
        pla
        tax
        pla
        rti

; The immediate VBI: row 0's registers (and the OS's shadows of them, which
; its second stage copies when SIO isn't busy), the rest, the sound.
; IRQs back on so the serial IRQ needn't wait out the VBI -- unless the VBI
; came in over an IRQ handler (the stacked P has I set): that IRQ isn't
; acknowledged yet, and CLI would take it again, nested, until the stack
; wraps. The OS decides its second stage the same way.
vbi:    tsx                     ; the stack: Y X A P PCL PCH
        lda     $0104,x
        and     #$04
        bne     @irq
        cli
@irq:   lda     #1
        sta     dli_row
        lda     #0
        sta     ATRACT
        lda     _row_chb
        sta     CHBASE
        sta     CHBAS
        ldx     #3
@r0:
        ldy     pfrow0,x
        lda     _row_pf,y
        sta     COLPF0,x
        sta     COLOR0,x
        lda     dl_cm,y
        sta     COLPM0,x
        sta     PCOLR0,x
        lda     dl_hp,y
        sta     HPOSP0,x
        lda     dl_sz,y
        sta     SIZEP0,x
        dex
        bpl     @r0
        lda     _dsp_colbk
        sta     COLOR4
        sta     COLBK
        lda     _dsp_prior
        sta     GPRIOR
        sta     PRIOR
        lda     _dsp_sizem
        sta     SIZEM
        lda     _dsp_hposm
        sta     HPOSM0
        lda     _dsp_hposm+1
        sta     HPOSM0+1
        lda     _dsp_hposm+2
        sta     HPOSM0+2
        lda     _dsp_hposm+3
        sta     HPOSM0+3
        jsr     _snd_vbi
        jmp     SYSVBV

; The deferred VBI: the players' rows, if a whole set is built.
vbid:   lda     _dsp_ready
        beq     @x
        ldx     #0
@cp:    lda     _row_flags,x
        sta     dl_flags,x
        lda     _row_hp,x
        sta     dl_hp,x
        lda     _row_hp+ROWS,x
        sta     dl_hp+ROWS,x
        lda     _row_hp+ROWS*2,x
        sta     dl_hp+ROWS*2,x
        lda     _row_hp+ROWS*3,x
        sta     dl_hp+ROWS*3,x
        lda     _row_cm,x
        sta     dl_cm,x
        lda     _row_cm+ROWS,x
        sta     dl_cm+ROWS,x
        lda     _row_cm+ROWS*2,x
        sta     dl_cm+ROWS*2,x
        lda     _row_cm+ROWS*3,x
        sta     dl_cm+ROWS*3,x
        lda     _row_sz,x
        sta     dl_sz,x
        lda     _row_sz+ROWS,x
        sta     dl_sz+ROWS,x
        lda     _row_sz+ROWS*2,x
        sta     dl_sz+ROWS*2,x
        lda     _row_sz+ROWS*3,x
        sta     dl_sz+ROWS*3,x
        inx
        cpx     #ROWS
        bne     @cp
        lda     #0
        sta     _dsp_ready
@x:     jmp     XITVBV
