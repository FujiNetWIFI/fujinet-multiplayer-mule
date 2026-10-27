;
; crt0.s -- start-up for the 7800 M.U.L.E. client: fujinet-lib's atari7800
; crt0 (bus/atari7800/crt0.s), with the claim naming SuperGame with cart
; RAM (kind 5 = A78MAP_SG_RAM + 1: the cartridge banks the image itself) and
; the NMI going straight to the display's DLI (nmi.s).
;
; Nothing here stores to $00-$1F: INPTCTRL stays unlocked, as the cart
; wants it. The loader starts us with DMA off and interrupts off; the C
; runtime's data and variables, and the console RAM's display lists and the
; cart RAM's character blocks (not cc65's BSS), are cleared here.
;
        .export         __STARTUP__ : absolute = 1
        .export         _exit
        .import         __CSTACK_START__, __CSTACK_SIZE__
        .import         __BLOCKS_START__
        .import         initlib, donelib, zerobss, copydata
        .import         push0, _main, _nmi, _irq

        .include        "zeropage.inc"

CTRL    = $3C
OFFSET  = $38

        .segment "STARTUP"
start:  sei
        cld
        lda     #$7F            ; MARIA's DMA off until the display is built
        sta     CTRL
        lda     #0
        sta     OFFSET
        ldx     #$FF
        txs
        ; zero page ($40-$FF), the console RAM but for zero page's and the
        ; stack's other faces ($2040-$20FF, $2140-$21FF), the cart's blocks
        lda     #0
        ldx     #$40
@zp:    sta     $00,x
        inx
        bne     @zp
        ldx     #$3F
@c2:    sta     $2000,x
        sta     $2100,x
        dex
        bpl     @c2
        lda     #$18
        ldx     #8
        jsr     clear
        lda     #$22
        ldx     #6
        jsr     clear
        lda     #>__BLOCKS_START__
        ldx     #$24
        jsr     clear
        lda     #<(__CSTACK_START__ + __CSTACK_SIZE__)
        sta     c_sp
        lda     #>(__CSTACK_START__ + __CSTACK_SIZE__)
        sta     c_sp+1
        jsr     zerobss
        jsr     copydata
        jsr     initlib
        jsr     push0           ; argc
        jsr     push0           ; argv
        ldy     #4
        jsr     _main
_exit:  jsr     donelib
@hang:  jmp     @hang

; X pages from page A, cleared
clear:  sta     ptr1+1
        lda     #0
        sta     ptr1
        tay
@p:     sta     (ptr1),y
        iny
        bne     @p
        inc     ptr1+1
        dex
        bne     @p
        rts

        .segment "CLAIM"
        .byte   "FUJI", 1, 5, 1, 0, 0, 0    ; version 1, SuperGame + RAM, cart RAM

        .segment "VECTORS"
        .byte   $FF, $87                    ; $FFF8/$FFF9: a 7800 cart
        .word   _nmi, start, _irq
