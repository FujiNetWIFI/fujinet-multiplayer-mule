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

        .segment "PMGFREE"      ; $B850-$BAFF: cleared by gfx_init
_row_flags: .res ROWS
_row_chb:   .res ROWS
_row_pf:    .res ROWS * 4
_row_hp:    .res ROWS * 4
_row_cm:    .res ROWS * 4
_row_sz:    .res ROWS * 4
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
        lda     _row_flags,x
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
        lda     _row_hp,x
        sta     HPOSP0
        lda     _row_hp+ROWS,x
        sta     HPOSP0+1
        lda     _row_hp+ROWS*2,x
        sta     HPOSP0+2
        lda     _row_hp+ROWS*3,x
        sta     HPOSP0+3
@cm:    lsr     dli_f
        bcc     @sz
        lda     _row_cm,x
        sta     COLPM0
        lda     _row_cm+ROWS,x
        sta     COLPM0+1
        lda     _row_cm+ROWS*2,x
        sta     COLPM0+2
        lda     _row_cm+ROWS*3,x
        sta     COLPM0+3
@sz:    lsr     dli_f
        bcc     @done
        lda     _row_sz,x
        sta     SIZEP0
        lda     _row_sz+ROWS,x
        sta     SIZEP0+1
        lda     _row_sz+ROWS*2,x
        sta     SIZEP0+2
        lda     _row_sz+ROWS*3,x
        sta     SIZEP0+3
@done:  inc     dli_row
        pla
        tax
        pla
        rti

; The immediate VBI: row 0's registers (and the OS's shadows of them, which
; its second stage copies when SIO isn't busy), the rest, the sound.
vbi:    cli                     ; the serial IRQ can't wait out the VBI
        lda     #1
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
        lda     _row_cm,y
        sta     COLPM0,x
        sta     PCOLR0,x
        lda     _row_hp,y
        sta     HPOSP0,x
        lda     _row_sz,y
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
