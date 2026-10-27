;
; probe.s -- M0: does the DLI protocol switch MARIA cleanly between 320A
; (text) and 160A (graphics) zones, CHARBASE, char width and colours per
; zone, with a sprite over both, and how much DMA may the zone before a
; switch have? run.sh builds it (gen.py makes the zones), runs it in MAME
; and check.py reads every line of the snapshot.
;
; The DLI for zone Z fires after the DMA of zone Z-1's last line (raster
; A); raster B shows that line and fetches Z's first, raster C shows it.
;   at once:  CHARBASE(Z); CTRL = CW(Z) | RM, the read mode already Z's
;             unless Z is 320A after 160A (a 320A -> 160A switch lands a
;             line early: the text row's last line, blank in the font but
;             for punctuation); then Z's colours into its pair (even zones
;             P0/P1, odd P2/P3: free, zone Z-2's)
;   160A -> 320A: WSYNC, WSYNC (raster C's start), CTRL = CW(Z) | RM(Z):
;             late by a line if the WSYNC comes after B has begun (the
;             zone before was heavy): the text row's first line, blank but
;             for punctuation
;
        .setcpu "6502"
        .import z_cb, z_ca, z_cb2, z_f, dl_rom
        .import z_pal0, z_pal1, z_pal2, z_pal3, z_pal4, z_pal5


BACKGRND = $20
WSYNC    = $24
MSTAT    = $28
DPPH     = $2C
DPPL     = $30
CHARBASE = $34
OFFSET   = $38
CTRL     = $3C
P4C2     = $32
P4C3     = $33

ZONES    = 24
DLBASE   = $1800
NULLDL   = $1E00
DLL      = $1E10

        .segment "ZEROPAGE"
zone:    .res 1
nx_cb:   .res 1
nx_ca:   .res 1
nx_cb2:  .res 1
nx_f:    .res 1
nx_pal:  .res 6
frames   = $FB              ; counted by the frame DLI (run.sh reads it)
cnt      = $FC              ; the idle loop's count (run.sh reads it)
ptr:     .res 2
ptr2:    .res 2
tmp:     .res 1
is_pal:  .res 1
lines:   .res 2

        .segment "CODE"
reset:  sei
        cld
        ldx     #$FF
        txs
        lda     #$7F
        sta     CTRL
        lda     #0
        sta     OFFSET
        ldx     #$40
@zp:    sta     $00,x
        inx
        bne     @zp
        ; clear the cart RAM
        lda     #$40
        ldx     #$40
        jsr     clrpages
        jsr     charsets
        ; the display lists to $1800
        lda     #<dl_rom
        sta     ptr
        lda     #>dl_rom
        sta     ptr+1
        lda     #<DLBASE
        sta     ptr2
        lda     #>DLBASE
        sta     ptr2+1
        ldx     #6              ; 6 pages = 24 x 64
        ldy     #0
@cp:    lda     (ptr),y
        sta     (ptr2),y
        iny
        bne     @cp
        inc     ptr+1
        inc     ptr2+1
        dex
        bne     @cp
        lda     #0
        sta     NULLDL
        sta     NULLDL+1
        jsr     tvdetect
        jsr     dllist
        lda     #>DLL
        sta     DPPH
        lda     #<DLL
        sta     DPPL
        ldx     #0
        stx     zone
        jsr     preload
        lda     #$00
        sta     BACKGRND
        lda     #$0E            ; P4: the sprite, white
        sta     P4C2
        sta     P4C3
@vb:    bit     MSTAT
        bpl     @vb
        lda     #$48
        sta     CTRL
idle:   inc     cnt
        bne     idle
        inc     cnt+1
        bne     idle
        inc     cnt+2
        jmp     idle

; clear X pages from page A
clrpages:
        sta     ptr+1
        lda     #0
        sta     ptr
        tay
@p:     sta     (ptr),y
        iny
        bne     @p
        inc     ptr+1
        dex
        bne     @p
        rts

; the glyphs: CB $40 code 1 = $FF; CB $48 code 1 = $55, 2 = $FF; CB $50
; codes 2-3 = $AA; the map rows $5800 (1), $5900 and $5A00 (2); the sprite
; $FF $FF at $6000 + line page
charsets:
        ldx     #7
@l:     txa
        clc
        adc     #$40
        sta     ptr+1
        lda     #0
        sta     ptr
        ldy     #1
        lda     #$FF
        sta     (ptr),y
        lda     ptr+1
        adc     #8
        sta     ptr+1
        lda     #$55
        sta     (ptr),y
        iny
        lda     #$FF
        sta     (ptr),y
        lda     ptr+1
        clc
        adc     #8
        sta     ptr+1
        lda     #$AA
        sta     (ptr),y
        iny
        sta     (ptr),y
        txa
        clc
        adc     #$60
        sta     ptr+1
        ldy     #0
        lda     #$FF
        sta     (ptr),y
        iny
        sta     (ptr),y
        dex
        bpl     @l
        ldx     #39
@m:     lda     #1
        sta     $5800,x
        lda     #2
        sta     $5900,x
        sta     $5A00,x
        dex
        bpl     @m
        rts

; NTSC or PAL: the lines between two blanks
tvdetect:
@a:     bit     MSTAT
        bpl     @a
@b:     bit     MSTAT
        bmi     @b
        lda     #0
        sta     lines
        sta     lines+1
@c:     sta     WSYNC
        inc     lines
        bne     @d
        inc     lines+1
@d:     bit     MSTAT
        bpl     @c
        lda     lines+1
        beq     @ntsc
        lda     lines
        cmp     #267 - 256
        bcc     @ntsc
        lda     #1
        .byte   $2C
@ntsc:  lda     #0
        sta     is_pal
        rts

; the DLL: the top blank (25 or 50 lines), the zones, the bottom
dllist: ldy     #0
        lda     is_pal
        bne     @pal
        lda     #15
        jsr     blank
        lda     #8
        jsr     blank
        jmp     @zones
@pal:   lda     #15
        jsr     blank
        lda     #15
        jsr     blank
        lda     #15
        jsr     blank
        lda     #1
        jsr     blank
@zones: ldx     #0
@z:     lda     #$87            ; DLI, 8 lines
        sta     DLL,y
        iny
        txa
        lsr     a
        lsr     a
        clc
        adc     #>DLBASE
        sta     DLL,y
        iny
        txa
        asl     a
        asl     a
        asl     a
        asl     a
        asl     a
        asl     a
        sta     DLL,y
        iny
        inx
        cpx     #ZONES
        bcc     @z
        lda     #$8F            ; the bottom: DLI (the frame), 16 lines
        jsr     blank
        lda     #15
        jsr     blank
        lda     #15
        jsr     blank
        lda     #15
        jsr     blank
        rts
blank:  sta     DLL,y
        iny
        lda     #>NULLDL
        sta     DLL,y
        iny
        lda     #<NULLDL
        sta     DLL,y
        iny
        rts

; zone X's values into nx_*
preload:
        lda     z_cb,x
        sta     nx_cb
        lda     z_ca,x
        sta     nx_ca
        lda     z_cb2,x
        sta     nx_cb2
        lda     z_f,x
        sta     nx_f
        lda     z_pal0,x
        sta     nx_pal
        lda     z_pal1,x
        sta     nx_pal+1
        lda     z_pal2,x
        sta     nx_pal+2
        lda     z_pal3,x
        sta     nx_pal+3
        lda     z_pal4,x
        sta     nx_pal+4
        lda     z_pal5,x
        sta     nx_pal+5
        rts

; nx_f: bit 7 the late switch, bit 6 the odd pair, bit 5 no colours
nmi:    pha
        lda     nx_cb
        sta     CHARBASE
        lda     nx_ca
        sta     CTRL
        bit     nx_f
        bmi     @late
        bvs     @odd
        lda     nx_f
        and     #$20
        bne     @next
        lda     nx_pal
        sta     $21
        lda     nx_pal+1
        sta     $22
        lda     nx_pal+2
        sta     $23
        lda     nx_pal+3
        sta     $25
        lda     nx_pal+4
        sta     $26
        lda     nx_pal+5
        sta     $27
        jmp     @next
@odd:   lda     nx_pal
        sta     $29
        lda     nx_pal+1
        sta     $2A
        lda     nx_pal+2
        sta     $2B
        lda     nx_pal+3
        sta     $2D
        lda     nx_pal+4
        sta     $2E
        lda     nx_pal+5
        sta     $2F
        jmp     @next
@late:  lda     nx_pal+1        ; the text's colour: pair A, colour 2
        bvs     @lodd
        sta     $22
        .byte   $2C
@lodd:  sta     $2A
        lda     nx_cb2
        sta     WSYNC
        sta     WSYNC
        sta     CTRL
@next:  txa
        pha
        ldx     zone
        inx
        cpx     #ZONES + 1
        bcc     @z
        inc     frames
        ldx     #0
@z:     stx     zone
        jsr     preload
        pla
        tax
        pla
        rti

irq:    rti

        .segment "CLAIM"
        .byte   "FUJI", 1, 0, 1, 0, 0, 0

        .segment "VECTORS"
        .byte   $FF, $87
        .word   nmi, reset, irq
