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
; of the beam. The OS skips it while SIO is busy and when the VBI came over
; an IRQ (often, while the stream comes in): the sprites' bitmaps wait for
; it too (sprite.s pmg_apply), so they and the rows that place them always
; change in the same frame.
;
; The charset and colours (row_chb, row_pf) the DLIs read as they are: a
; row whose flags say they don't change would keep the row above's for as
; long as the copy is put off, so whatever changes them marks the row and
; the one below (pf_mark) until the flags are made again. A new screen is
; built with the playfield off (gfx.c screen_start, dsp_hold), and comes on
; with the first copy of its flags.
;
; The 5200 (src/atari5200) assembles this too: no OS there, so the RAM
; vectors are set by hand, the VBI saves its own registers, and the crt0's
; copy of the BIOS's VBI (the clock, the shadows, the pots) stands in for
; the OS's stages.
;
        .export _dsp_init, _dli_on, _dsp_hold, pf_mark, pm_rows
        .export _row_flags, _row_chb, _row_pf, _row_hp, _row_cm, _row_sz
        .export _dsp_colbk, _dsp_prior, _dsp_sizem, _dsp_hposm, _dsp_ready
        .import _snd_vbi, _bm_ready, pmg_apply

ROWS    = 24

        .include "hw.inc"
.ifdef __ATARI5200__
        .import vbi_body, vbi_exit      ; crt0.s: the BIOS's VBI past its pushes; its end
VVBLKI  = $0202
VVBLKD  = $0204
VDSLST  = $0206
PCOLR0  = $08
COLOR0  = $0C
COLOR4  = $10
ATRACT  = $04
SDMCTL  = $07
.else
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
.endif

.ifdef __ATARI5200__
        .segment "ZEROPAGE"
dli_pf2:    .res 1              ; the next row's PF2 and CHBASE (dli)
dli_chb:    .res 1
.endif

        .segment "BSS"          ; the players' rows as built
_row_hp:    .res ROWS * 4
_row_cm:    .res ROWS * 4
        .segment "LOWBSS"       ; (low memory, mule-atari.cfg: MAIN is full)
_row_flags: .res ROWS
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
_dsp_hold:  .res 1              ; 1: the playfield is off for a new screen; 2: on at the next copy
_dli_on:    .res 1
dli_row:    .res 1
dli_f:      .res 1
pm_r:       .res 1

        .code

        .rodata
pfrow0: .byte   0, ROWS, ROWS*2, ROWS*3

        .code

; pf_mark: row X's charset or colours changed: its DLI and the next row's
; load them, in the flags shown and the flags built, till the flags are
; made again (A, X, Y kept)
pf_mark:
        pha
        lda     #1              ; RF_PF
        ora     dl_flags,x
        sta     dl_flags,x
        lda     #1
        ora     _row_flags,x
        sta     _row_flags,x
        cpx     #ROWS - 1
        bcs     @r
        lda     #1
        ora     dl_flags+1,x
        sta     dl_flags+1,x
        lda     #1
        ora     _row_flags+1,x
        sta     _row_flags+1,x
@r:     pla
        rts

; pm_rows: player Y's rows A..A+3 shown as built at once, for terrain drawn
; straight into a player (mapgfx.s: its bytes are there now, and the copy
; may be frames off); their DLIs and the next row's load the players
pm_rows:
        sta     pm_r
        lda     pfrow0,y        ; p * ROWS
        clc
        adc     pm_r
        tay
        ldx     #4
@c:     lda     _row_hp,y
        sta     dl_hp,y
        lda     _row_cm,y
        sta     dl_cm,y
        lda     _row_sz,y
        sta     dl_sz,y
        iny
        dex
        bne     @c
        ldx     pm_r
        ldy     #5
@f:     cpx     #ROWS
        bcs     @x
        lda     #$0E            ; RF_HPOS | RF_COL | RF_SIZE
        ora     dl_flags,x
        sta     dl_flags,x
        lda     #$0E
        ora     _row_flags,x
        sta     _row_flags,x
        inx
        dey
        bne     @f
@x:     rts

; void dsp_init(void): the interrupts (the display list is gfx.c's)
_dsp_init:
        lda     #0
        sta     NMIEN
        lda     #<dli
        sta     VDSLST
        lda     #>dli
        sta     VDSLST+1
.ifdef __ATARI5200__
        lda     #<vbi
        sta     VVBLKI
        lda     #>vbi
        sta     VVBLKI+1
        lda     #<vbid
        sta     VVBLKD
        lda     #>vbid
        sta     VVBLKD+1
.else
        lda     #6              ; immediate VBI
        ldy     #<vbi
        ldx     #>vbi
        jsr     SETVBV
        lda     #7              ; deferred VBI
        ldy     #<vbid
        ldx     #>vbid
        jsr     SETVBV
.endif
        lda     #$C0
        sta     NMIEN
        rts

; A row's DLI: PF2 (a text row's background) goes in first thing after
; WSYNC; the rest follows as row_flags asks.
;
; On the 5200 the row's PF2 and CHBASE wait in zero page (the last DLI, or
; the VBI, put them there), so the WSYNC comes ten cycles in and CHBASE
; goes in with PF2, in the horizontal blank. Looked up from the tables, a
; text row's last line (ANTIC takes most of its cycles) ends before the
; WSYNC, and CHBASE lands a line late; MAME's ANTIC takes CHBASE once a
; row, at the display-list fetch (antic.cpp), so there the whole row came
; out in the last row's charset.
.ifdef __ATARI5200__
dli:    pha
        lda     dli_pf2
        sta     WSYNC
        sta     COLPF0+2
        lda     dli_chb
        sta     CHBASE
        txa
        pha
        ldx     dli_row
.else
dli:    pha
        txa
        pha
        ldx     dli_row
        lda     _row_pf+ROWS*2,x
        sta     WSYNC
        sta     COLPF0+2
.endif
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
@done:
.ifdef __ATARI5200__
        lda     _row_pf+ROWS*2+1,x      ; the next row's, ready
        sta     dli_pf2
        lda     _row_chb+1,x
        sta     dli_chb
.endif
        inc     dli_row
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
vbi:
.ifdef __ATARI5200__
        pha                     ; (the BIOS's NMI pushes nothing)
        txa
        pha
        tya
        pha
.endif
        tsx                     ; the stack: Y X A P PCL PCH
        lda     $0104,x
        and     #$04
        bne     @irq
        cli
@irq:   lda     #1
        sta     dli_row
.ifdef __ATARI5200__
        lda     _row_pf+ROWS*2+1        ; row 1's, for its DLI
        sta     dli_pf2
        lda     _row_chb+1
        sta     dli_chb
.endif
        lda     #0
        sta     ATRACT
        lda     _row_chb
        sta     CHBASE
.ifndef __ATARI5200__
        sta     CHBAS
.endif
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
.ifndef __ATARI5200__
        sta     GPRIOR
.endif
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
.ifdef __ATARI5200__
        jmp     vbi_body
.else
        jmp     SYSVBV
.endif

; The deferred VBI: the sprites' bitmaps and the players' rows, if a whole
; set is built; the playfield back on for a new screen. Row 0's players
; first: the immediate VBI gave them the last set's, and rows that don't
; move a player keep the position row 0 gave it.
vbid:   lda     _dsp_ready
        bne     @go
        jmp     @x
@go:    ldx     #3
@r0:    ldy     pfrow0,x
        lda     _row_hp,y
        sta     HPOSP0,x
        lda     _row_cm,y
        sta     COLPM0,x
        sta     PCOLR0,x
        lda     _row_sz,y
        sta     SIZEP0,x
        dex
        bpl     @r0
        lda     _bm_ready
        beq     @rows
        jsr     pmg_apply
@rows:  ldx     #0
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
        lda     _dsp_hold
        cmp     #2
        bne     @x
        lda     #$3E            ; gfx.c gfx_init's
        sta     SDMCTL
        sta     DMACTL
        lda     #0
        sta     _dsp_hold
.ifdef __ATARI5200__
@x:     jmp     vbi_exit
.else
@x:     jmp     XITVBV
.endif
