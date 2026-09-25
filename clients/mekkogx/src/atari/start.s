;
; start.s -- the load chunk that runs before the program is loaded
; (mule-atari.cfg: INITAD). It switches BASIC off on an XL/XE, makes sure
; there is RAM up to $BFFF (48K, no cartridge), blanks the screen while the
; rest loads over it, and puts MEMTOP under the graphics so cc65's C stack
; starts there.
;
        .export mule_start
        .import __GFX_START__

PORTB   = $D301
RAMTOP  = $6A
BASICF  = $03F8
MEMTOP  = $02E5
SDMCTL  = $022F
CIOV    = $E456

        .segment "MULESTART"

mule_start:
        lda     PORTB           ; XL/XE: BASIC off (bit 1 set). On an 800
        ora     #$02            ; port B is the joysticks' and ignores it
        sta     PORTB
        lda     #1
        sta     BASICF          ; ... and off after RESET too
        lda     #$C0
        sta     RAMTOP
        ; is $A000-$BFFF RAM?
        ldx     #$55
        stx     $BFFF
        cpx     $BFFF
        bne     no_ram
        ldx     #$AA
        stx     $A000
        cpx     $A000
        bne     no_ram
        lda     #0
        sta     SDMCTL
        lda     #<(__GFX_START__ - 1)
        sta     MEMTOP
        lda     #>(__GFX_START__ - 1)
        sta     MEMTOP+1
        rts

no_ram: ldx     #0              ; IOCB 0 (E:): PUT RECORD
        lda     #$09
        sta     $0342
        lda     #<msg
        sta     $0344
        lda     #>msg
        sta     $0345
        lda     #<(msg_end - msg)
        sta     $0348
        lda     #0
        sta     $0349
        jsr     CIOV
hang:   jmp     hang

msg:    .byte   "M.U.L.E. NEEDS 48K AND NO CARTRIDGE", $9B
msg_end:
