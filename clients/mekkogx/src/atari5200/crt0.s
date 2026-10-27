;
; crt0.s -- the 5200 M.U.L.E.'s start, from fujinet-lib-experimental's
; bus/atari5200/crt0.s (FNLIB_5200), for a Super Cart.
;
; The header's $BFFD is $FF, so the BIOS jumps straight here with nothing
; set up and the last bank in (the BIOS's read of $BFFE picked it). This
; clears the chips and all 16K of RAM, copies what the screens show from
; the banks they are in into RAM (the charsets, the sprite shapes and
; tables: ld65's load/run segments, mule-a52.cfg), sets the RAM vectors at
; $0200-$021B, and starts the keypad's IRQ and the VBI, whose zero page is
; the BIOS's ($00-$18: the clock, the shadows, the pots). Display DMA
; stays off until gfx_init has a display list.
;
; The console's NMI and IRQ vectors are the BIOS's, and both revisions
; enter through $FCA2 / $FC00 to JMP (VVBLKI) / JMP (VIMIRQ) with nothing
; pushed: these handlers save what they use. gfxa.s (src/atari) puts its
; own VBIs in front: its immediate one ends in vbi_body, its deferred one
; in vbi_exit.

        .export         __STARTUP__ : absolute = 1
        .export         _exit, start, _fuji_a5200_key, vbi_body, vbi_exit
        .import         __CSTACK_START__, __CSTACK_SIZE__
        .import         __SHAPES_LOAD__, __SHAPES_RUN__, __SHAPES_SIZE__
        .import         __FONT_LOAD__, __FONT_RUN__, __FONT_SIZE__
        .import         __MAPCHARS_LOAD__, __MAPCHARS_RUN__, __MAPCHARS_SIZE__
        .import         __TOWNCHARS_LOAD__, __TOWNCHARS_RUN__, __TOWNCHARS_SIZE__
        .import         initlib, donelib, copydata, callmain

        .include        "zeropage.inc"

POKMSK  = $00
RTCLOKH = $01
RTCLOKL = $02
CRITIC  = $03
ATRACT  = $04
SDLSTL  = $05
SDLSTH  = $06
SDMCTL  = $07
PCOLR0  = $08
PADDL0  = $11
VIMIRQ  = $0200
VVBLKD  = $0204
VKYBDI  = $0208
VKYBDF  = $020A
GTIA    = $C000
COLPM0  = $C012
CONSOL  = $C01F
ANTIC   = $D400
DMACTL  = $D400
DLISTL  = $D402
DLISTH  = $D403
NMIEN   = $D40E
POKEY   = $E800
POT0    = $E800
KBCODE  = $E809
POTGO   = $E80B
IRQEN   = $E80E
IRQST   = $E80E
SKCTL   = $E80F
BANKSEL = $BFD0                 ; + 4n: bank n
HOLD_FRAMES = 4

        .segment "STARTUP"
start:  sei
        cld
        ldx     #$FF
        txs
        lda     #0
        sta     NMIEN
        sta     DMACTL
        sta     IRQEN
        tax
@clr:   sta     POKEY,x         ; every chip register to 0, as the BIOS does
        sta     GTIA,x
        sta     ANTIC,x
        sta     $00,x
        sta     $0100,x
        inx
        bne     @clr
        lda     #$02            ; RAM $0200-$3FFF, through $00/$01
        sta     $01
        ldy     #0
        tya
@ram:   sta     ($00),y
        iny
        bne     @ram
        inc     $01
        ldx     $01
        cpx     #$40
        bne     @ram
        sta     $01
        ldx     #vecs_end - vecs - 1
@vec:   lda     vecs,x
        sta     VIMIRQ,x
        dex
        bpl     @vec
        lda     #$04            ; the pots on, controller 1's keypad
        sta     CONSOL
        ldx     #(copies_end - copies) - 7
@cp:    ldy     copies+6,x
        lda     BANKSEL,y
        jsr     copy
        txa
        sec
        sbc     #7
        tax
        bcs     @cp
        lda     #<(__CSTACK_START__ + __CSTACK_SIZE__)
        sta     c_sp
        lda     #>(__CSTACK_START__ + __CSTACK_SIZE__)
        sta     c_sp+1
        jsr     copydata
        lda     #$02            ; keypad scan on
        sta     SKCTL
        lda     #$40            ; its IRQ, alone
        sta     POKMSK
        sta     IRQEN
        lda     #$C0            ; VBI and DLIs, as the BIOS leaves NMIEN
        sta     NMIEN
        cli
        jsr     initlib
        jsr     callmain
_exit:  jsr     donelib
@hang:  jmp     @hang

; copies+X: load, run, size -> memcpy (X kept; its bank is in)
copy:   lda     copies,x
        sta     ptr1
        lda     copies+1,x
        sta     ptr1+1
        lda     copies+2,x
        sta     ptr2
        lda     copies+3,x
        sta     ptr2+1
        lda     copies+5,x
        sta     tmp1            ; whole pages
        ldy     #0
@pg:    lda     tmp1
        beq     @part
@b:     lda     (ptr1),y
        sta     (ptr2),y
        iny
        bne     @b
        inc     ptr1+1
        inc     ptr2+1
        dec     tmp1
        jmp     @pg
@part:  lda     copies+4,x
        beq     @x
        sta     tmp1
@p:     lda     (ptr1),y
        sta     (ptr2),y
        iny
        cpy     tmp1
        bne     @p
@x:     rts

        .rodata

; load, run, size, the bank * 4 (mule-a52.cfg)
copies: .word   __SHAPES_LOAD__, __SHAPES_RUN__, __SHAPES_SIZE__
        .byte   3 * 4
        .word   __FONT_LOAD__, __FONT_RUN__, __FONT_SIZE__
        .byte   1 * 4
        .word   __MAPCHARS_LOAD__, __MAPCHARS_RUN__, __MAPCHARS_SIZE__
        .byte   1 * 4
        .word   __TOWNCHARS_LOAD__, __TOWNCHARS_RUN__, __TOWNCHARS_SIZE__
        .byte   1 * 4
copies_end:

vecs:   .word   irq, vbi, vbi_exit, dli, kbd, kbd_done
        .word   pla_rti, pla_rti, pla_rti, pla_rti, pla_rti, pla_rti, pla_rti, pla_rti
vecs_end:

keys:   .byte   $FF, $0B, $00, $0A, $0E, $09, $08, $07
        .byte   $0D, $06, $05, $04, $0C, $03, $02, $01

        .code

; The BIOS's $FCB8, register for register.
vbi:    pha
        txa
        pha
        tya
        pha
vbi_body:
        inc     RTCLOKL
        bne     :+
        inc     ATRACT
        inc     RTCLOKH
:       lda     hold            ; a held key's repeats keep this above zero
        beq     :+
        dec     hold
:       lda     CRITIC
        bne     vbi_exit
        lda     SDLSTH
        sta     DLISTH
        lda     SDLSTL
        sta     DLISTL
        lda     SDMCTL
        sta     DMACTL
        ldy     ATRACT
        bpl     :+
        ldy     #$80
        sty     ATRACT
:       ldx     #8
@col:   lda     PCOLR0,x
        cpy     #$80
        bcc     :+
        eor     RTCLOKH
        and     #$F6
:       sta     COLPM0,x
        dex
        bpl     @col
        ldx     #7
@pot:   lda     POT0,x
        sta     PADDL0,x
        dex
        bpl     @pot
        sta     POTGO
        jmp     (VVBLKD)

vbi_exit:
        pla
        tay
        pla
        tax
        pla
        rti

dli:    rti

; Only the keypad's IRQ is enabled here; anything else is acknowledged.
irq:    pha
        lda     IRQST
        and     #$40
        bne     @other
        lda     #$BF
        sta     IRQEN
        lda     POKMSK
        sta     IRQEN
        jmp     (VKYBDI)
@other: lda     #0
        sta     IRQEN
        lda     POKMSK
        sta     IRQEN
        pla
        rti

pla_rti:
        pla
        rti

; The BIOS's $FD02: KBCODE through its table, then VKYBDF.
kbd:    txa
        pha
        tya
        pha
        lda     KBCODE
        lsr     a
        and     #$0F
        tax
        lda     keys,x
        jmp     (VKYBDF)

; 0-9, $0A '*', $0B '#', $0C START, $0D PAUSE, $0E RESET; stored + 1.
; Without debounce (SKCTL $02, the BIOS's) POKEY interrupts on every scan of
; a held key, and SKSTAT's key-down bit drops between scans: a repeat of the
; key while `hold` runs only restarts it, so each press counts once.
kbd_done:
        ldx     #HOLD_FRAMES
        cmp     held
        bne     @new
        ldy     hold
        stx     hold
        cpy     #0
        bne     @same
@new:   stx     hold
        sta     held
        clc
        adc     #1
        sta     _fuji_a5200_key
        lda     #0
        sta     ATRACT
@same:  jmp     vbi_exit

        .bss
_fuji_a5200_key:
        .res    1
held:   .res    1               ; the last key
hold:   .res    1               ; frames until it may count again

        .segment "CLAIM"
        .byte   "FUJI", 1, 4, 0             ; version 1, kind 4: a Super Cart
        .byte   $02                         ; $BFE7: PAL-compatible, for the 2-port BIOS

        .segment "CARTHDR"
        .byte   "M.U.L.E.            "      ; $BFE8: 20 characters, unseen
        .byte   $FF, $FF                    ; $BFFC: year; $BFFD = $FF: no BIOS title
        .word   start                       ; $BFFE: entry
