;
; nmi.s -- the display's interrupts: a DLI where a zone needs MARIA set
; otherwise than the zone above (the tables gfx.c's mt_tables makes), and
; the frame's DLI on the blank below the screen, which keeps the clock,
; plays the sound and shows what maria.s built.
;
; The DLI for zone Z comes right after MARIA has fetched zone Z-1's last
; line (raster A); raster B shows that line and fetches Z's first, raster C
; shows it. MAME applies the read mode and the colours a line at a time, as
; the line's DMA starts (7 CPU cycles in). So (tools/a78sim/probe, M0):
;   at once   CHARBASE and CTRL, the read mode already zone Z's unless Z is
;             320A under a 160A zone: a 320A -> 160A switch lands a line
;             early, on a text row's last line, which the font leaves blank
;             but for some punctuation
;   then      Z's colours into its pair: P0/P1 for an even zone, P2/P3 for
;             an odd one, whose zone (Z-2) is long gone
;   160A -> 320A: the text's colour, WSYNC, WSYNC (raster C's start), the
;             read mode. A line late if the zone above was heavy (over ~200
;             cycles of DMA), on the text's first line: blank too
; MARIA's own WSYNC ($24); nothing here touches the TIA.
;
; A PAL console's hues sit one higher than an NTSC one's: the zones'
; colours come so from gfx.c (mt_tables), BACKGRND's and P4-P7's go
; through palfix. The clock is frame_count in jiffies of 1/60
; s however fast the console
; runs: NTSC (59.958 Hz) counts one more every 1421 frames, PAL (49.920)
; six for five frames and one more every 523.
;
        .include "a78.inc"
        .export _nmi, _irq, _frame_count, _tbl_pending, _dsp_colbk
        .export _nx_cb, _nx_ca, _nx_cb2, _nx_f, _nx_p, _nx_dli, _nx_n
        .export _spr_pal, _ter_pal, _mt_start, palfix
        .export _lv_cb := lv_cb, _lv_ca := lv_ca, _lv_f := lv_f, _dll_off := dll_off
        .import _mt_dll, _mt_top, _mt_ntsc, _zfront, _zflip, _dsp_ready
        .import slot_lo, slot_hi, _snd_vbi

NDLI    = 26            ; 24 zones, the frame's, and one to spare

        .segment "ZEROPAGE"
dcb:    .res 1          ; the next DLI's CHARBASE, CTRL, flags
dca:    .res 1
df:     .res 1          ; 7 late (160A -> 320A), 6 odd zone, 5 the frame, 4 no colours
dord:   .res 1          ; and its place in the tables

        .segment "CONSBSS"      ; (console RAM: read in the DLI)
lv_cb:  .res NDLI
lv_ca:  .res NDLI
lv_cb2: .res NDLI
lv_f:   .res NDLI
lv_p:   .res NDLI * 6   ; [i * NDLI + k]

        .bss
_frame_count: .res 2
_nx_cb:  .res NDLI      ; the next set of tables (gfx.c), taken with a set
_nx_ca:  .res NDLI      ; of display lists
_nx_cb2: .res NDLI
_nx_f:   .res NDLI
_nx_p:   .res NDLI * 6
_nx_dli: .res ROWS      ; zone has a DLI
_nx_n:   .res 1
_tbl_pending: .res 1
_dsp_colbk: .res 1
_spr_pal: .res 4        ; P4-P7 colours 2 and 3: the sprites'
_ter_pal: .res 4        ; and colour 1: the terrain's
dll_off: .res ROWS      ; zone z's DLL entry: _mt_dll + dll_off[z]
c5:     .res 1
cfix:   .res 2
tmp:    .res 1

        .code

; void mt_start(void): the DLL's offsets, the tables gfx.c made (it calls
; this once they are), the clock; DMA on in the next blank
_mt_start:
        ldx     #0
        lda     _mt_top
        sta     tmp
        asl     a
        adc     tmp             ; 3 * top
@o:     sta     dll_off,x
        clc
        adc     #3
        inx
        cpx     #ROWS
        bcc     @o
        lda     #0
        sta     _frame_count
        sta     _frame_count+1
        sta     c5
        jsr     fixreset
        jsr     install
        jsr     load0
        lda     #0
        sta     _tbl_pending
@vb:    bit     MSTAT           ; in a blank: DMA on, CTRL as zone 0's DLI will
        bpl     @vb
        lda     lv_ca
        sta     CTRL
        rts

fixreset:
        lda     _mt_ntsc
        beq     @pal
        lda     #<1421
        sta     cfix
        lda     #>1421
        sta     cfix+1
        rts
@pal:   lda     #<523
        sta     cfix
        lda     #>523
        sta     cfix+1
        rts

; the next tables into the DLI's, the DLI bits into the DLL
install:
        ldx     _nx_n
        dex
@t:     lda     _nx_cb,x
        sta     lv_cb,x
        lda     _nx_ca,x
        sta     lv_ca,x
        lda     _nx_cb2,x
        sta     lv_cb2,x
        lda     _nx_f,x
        sta     lv_f,x
        lda     _nx_p,x
        sta     lv_p,x
        lda     _nx_p+NDLI,x
        sta     lv_p+NDLI,x
        lda     _nx_p+NDLI*2,x
        sta     lv_p+NDLI*2,x
        lda     _nx_p+NDLI*3,x
        sta     lv_p+NDLI*3,x
        lda     _nx_p+NDLI*4,x
        sta     lv_p+NDLI*4,x
        lda     _nx_p+NDLI*5,x
        sta     lv_p+NDLI*5,x
        dex
        bpl     @t
        ldy     #ROWS - 1
@d:     lda     #$07
        ldx     _nx_dli,y
        beq     @nd
        lda     #$87
@nd:    ldx     dll_off,y
        sta     _mt_dll,x
        dey
        bpl     @d
        rts

; the first DLI's (zone 0's) values
load0:  ldx     #0
        stx     dord
load:   lda     lv_cb,x
        sta     dcb
        lda     lv_ca,x
        sta     dca
        lda     lv_f,x
        sta     df
        rts

; ---- the DLI --------------------------------------------------------------------
_nmi:   pha
        lda     dcb
        sta     CHARBASE
        lda     dca
        sta     CTRL
        txa
        pha
        ldx     dord
        bit     df
        bmi     late
        bvs     odd
        lda     df
        and     #$30
        bne     special
        lda     lv_p,x
        sta     $21
        lda     lv_p+NDLI,x
        sta     $22
        lda     lv_p+NDLI*2,x
        sta     $23
        lda     lv_p+NDLI*3,x
        sta     $25
        lda     lv_p+NDLI*4,x
        sta     $26
        lda     lv_p+NDLI*5,x
        sta     $27
        jmp     next
odd:    lda     df
        and     #$30
        bne     special
        lda     lv_p,x
        sta     $29
        lda     lv_p+NDLI,x
        sta     $2A
        lda     lv_p+NDLI*2,x
        sta     $2B
        lda     lv_p+NDLI*3,x
        sta     $2D
        lda     lv_p+NDLI*4,x
        sta     $2E
        lda     lv_p+NDLI*5,x
        sta     $2F
        jmp     next
late:   lda     lv_p+NDLI,x     ; the text's colour: pair A, colour 2
        bvs     @odd
        sta     $22
        bvc     @w
@odd:   sta     $2A
@w:     lda     lv_cb2,x
        sta     WSYNC
        sta     WSYNC
        sta     CTRL
next:   inx
        stx     dord
        jsr     load
        pla
        tax
        pla
_irq:   rti
special:
        and     #$20
        beq     next            ; no colours
        ; ---- the frame ----
        ; zone 0's values first: should this run on into the next frame's
        ; DLIs, they nest in it with the tables in order (a frame's
        ; glitch at worst, never every zone after off by one)
        tya
        pha
        jsr     load0
        jsr     clock
        jsr     _snd_vbi
        lda     _dsp_ready
        beq     @done
        ldy     #ROWS - 1       ; the zones just built
@f:     lda     _zflip,y
        beq     @nf
        lda     #0
        sta     _zflip,y
        lda     _zfront,y
        eor     #1
        sta     _zfront,y
        sta     tmp
        tya
        asl     a
        ora     tmp
        tax                     ; z * 2 + the slot
        lda     slot_hi,x
        pha
        lda     slot_lo,x
        ldx     dll_off,y
        sta     _mt_dll+2,x
        pla
        sta     _mt_dll+1,x
@nf:    dey
        bpl     @f
        lda     _dsp_colbk
        jsr     palfix
        sta     BACKGRND
        ldx     #3              ; P4-P7
        ldy     #12
@p:     lda     _ter_pal,x
        jsr     palfix
        sta     P4C1,y
        lda     _spr_pal,x
        jsr     palfix
        sta     P4C1+1,y
        sta     P4C1+2,y
        dey
        dey
        dey
        dey
        dex
        bpl     @p
        lda     _tbl_pending
        beq     @nt
        jsr     install
        lda     #0
        sta     _tbl_pending
@nt:    lda     #0
        sta     _dsp_ready
@done:  pla
        tay
        pla
        tax
        pla
        rti

; A's colour as a PAL console shows it: its hue one on (grey and hue 15
; as they are). X, Y kept.
palfix: pha
        lda     _mt_ntsc
        bne     @n
        pla
        cmp     #$10
        bcc     @r
        cmp     #$F0
        bcs     @r
        adc     #$10            ; (carry clear)
@r:     rts
@n:     pla
        rts

; frame_count: 60 a second
clock:  inc     _frame_count
        bne     @c1
        inc     _frame_count+1
@c1:    lda     _mt_ntsc
        bne     @fix
        inc     c5              ; PAL: a sixth jiffy every five frames
        lda     c5
        cmp     #5
        bcc     @fix
        lda     #0
        sta     c5
        jsr     @one
@fix:   lda     cfix
        bne     @dl
        dec     cfix+1
@dl:    dec     cfix
        lda     cfix
        ora     cfix+1
        bne     @r
        jsr     fixreset
@one:   inc     _frame_count
        bne     @r
        inc     _frame_count+1
@r:     rts
