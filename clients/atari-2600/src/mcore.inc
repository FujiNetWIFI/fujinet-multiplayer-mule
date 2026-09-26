; mcore.inc -- the contents of the fixed tail. From the Battleship 2600
; client's bscore.inc/bstrans.inc; the transport is the same code.

; MGOTO must be first: mdefs.inc gives it its address.
        ORG     MGOTO

; ---------------------------------------------------------------------------
; MGOTO -- select bank A and enter it at $1000, with a fresh stack.
; `sta FNRSEL,x`: the base's low byte is $00, so the index never carries and
; the dummy read lands on the written address.
        clc
        adc     #FH_BANK
        tax
        sta     FNRSEL,x
        ldx     #$FF            ; a switch is a jump: nothing returns through
        txs                     ;   one, so the stack starts again here
        jmp     $1000

; ---------------------------------------------------------------------------
; FNRW -- register X = A. Two stores: arm (data ignored), then commit.
FNRW:   sta     FNRSEL,x
        sta     FNCMT
        rts

; FNARM -- open the decode gate: two stores carrying two values, in order.
FNARM:  lda     #FNAM1
        sta     FNRSEL+FH_ARM1
        lda     #FNAM2
        sta     FNRSEL+FH_ARM2
        rts

; FNCHK -- Z set if a FujiNet cartridge is answering.
FNCHK:  lda     FNMAG0
        cmp     #'F'
        bne     FNCHK1
        lda     FNMAG1
        cmp     #'N'
FNCHK1: rts

; FNBEG -- begin a transaction described by FNDEV / FNCMD / FNNPR. Rewinds
; the TX stream last, so parameters may follow at once.
FNBEG:  lda     #16             ; ~9s, longer than the cart's own 5s
        sta     FNTMO
        lda     FNDEV
        ldx     #FR_DEV
        jsr     FNRW
        lda     FNCMD
        ldx     #FR_CMD
        jsr     FNRW
        lda     FNNPR
        ldx     #FR_NPAR
        jsr     FNRW
        lda     #0
        ldx     #FR_DRST
        jmp     FNRW

; FNPB -- append a one-byte parameter A.
FNPB:   ldx     #1
        stx     FNTX
        sta     FNTX
        rts

; FNPW -- append a two-byte parameter, A low, X high.
FNPW:   ldy     #2
        sty     FNTX
        sta     FNTX
        stx     FNTX
        rts

; FNLNCH -- launch the transaction: SEQ = the CART's ACKSEQ + 1 (never a RAM
; counter: RESET restarts the console, not the cart), kept in FNSEQ.
FNLNCH: lda     FNACKS
        clc
        adc     #1
        bne     FNL1
        lda     #1              ; 0 means "never used"
FNL1:   sta     FNSEQ
        ldx     #FR_SEQ
        jmp     FNRW

; FNGO -- launch and wait, blind. A = FNE* (0 = done). For the boot's
; appkey calls; the game's requests step through bank 2 instead.
FNGO:   jsr     FNLNCH
        lda     FNTMO
        sta     FNCNT
FNGO2:  ldy     #0
FNGO3:  ldx     #0
FNGO4:  lda     FNACKS          ; published last: a match means all is there
        cmp     FNSEQ
        beq     FNGO5
        dex
        bne     FNGO4
        dey
        bne     FNGO3
        dec     FNCNT
        bne     FNGO2
        lda     #FNEWAIT
        rts
FNGO5:  lda     FNERR
        rts

; FNACK -- 0 on success, FNENAK if the server said no.
FNACK:  lda     FNRCMD
        cmp     #$06
        beq     FNACK1
        lda     #FNENAK
        rts
FNACK1: lda     #FNEOK
        rts

; ---------------------------------------------------------------------------
; Text, composed by the cartridge.
FNROWA: sta     FNRSEL+FH_TROW  ; begin text row A
        rts
FNCHR:  sta     FNRSEL+FH_TCHR  ; append character A
        rts
FNSPC:  tax                     ; append A spaces
FNSPC1: lda     #' '
        sta     FNRSEL+FH_TCHR
        dex
        bne     FNSPC1
        rts

; ---------------------------------------------------------------------------
; MCOLD -- power-on and RESET. Tiny: the arming pair and the switch, which
; must be reachable whatever bank the console was left in (the RESET switch
; restarts the 6507 but not the cartridge). Bank 0 does the rest.
MCOLD:  sei
        cld
        ldx     #$FF
        txs
        lda     #ENCOLD         ; RAM survives a reset: force a cold entry
        sta     MENT
        lda     #FNAM1
        sta     FNRSEL+FH_ARM1
        lda     #FNAM2
        sta     FNRSEL+FH_ARM2
        lda     #BKBOOT
        jmp     MGOTO

        ORG     $1FFC
        DW      MCOLD
        DW      MCOLD
