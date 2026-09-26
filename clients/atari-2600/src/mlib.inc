; mlib.inc -- the per-bank library: text, blits, path buffers, input,
; numbers. CODE, included inside a bank's ORG. A bank switch replaces all of
; $1000-$17FF, so every bank carries its own copy of what it calls; the
; MHAS* flags leave out what a bank does not.
;
; From the Battleship 2600 client's bslib.inc. The control-page rules hold
; here too: only STA/STX/STY to $1D/$1E, never a read, never RMW, never an
; indirect store (tools/checkrom.py fails the build on them).

        IFNDEF  MHASTXT
MHASTXT EQU     1
        ENDIF
        IF      MHASTXT
; FNCLS -- blank every text row. The planes survive a bank switch, so a
; screen that does not paint a row inherits the last screen's.
FNCLS:  ldx     #0
FNCLS1: txa
        jsr     FNROWA
        jsr     FNENDW
        inx
        cpx     #FNTROW
        bcc     FNCLS1
        rts

; FNSETP -- point FNPTRL/H at X (lo) / Y (hi).
FNSETP: stx     FNPTRL
        sty     FNPTRH
        rts

; FNSTRA -- append the NUL-terminated string at FNPTRL/H to the row being
; composed (12 at most). Clobbers Y.
FNSTRA: ldy     #0
FNSA1:  lda     (FNPTRL),y
        beq     FNSA2
        sta     FNRSEL+FH_TCHR
        iny
        cpy     #FNTCOL
        bne     FNSA1
FNSA2:  rts

; FNSTRXY -- FNSETP then FNSTRA.
FNSTRXY: jsr    FNSETP
        jmp     FNSTRA

; FNENDW -- render the composed row and wait (bounded) for FNBTXG to say it
; landed: on hardware the render is on the cart's other core.
; Preserves X.
FNENDW: txa
        pha
        ldx     FNBTXG
        lda     #0
        sta     FNRSEL+FH_TEND
        ldy     #128
FNEW1:  cpx     FNBTXG
        bne     FNEW2
        dey
        bne     FNEW1
FNEW2:  pla
        tax
        rts

        ENDIF

; FNBLIT -- fire blit transform A and wait for FNBGEN. The cart has one
; blit slot and no queue: the next blit's argument stores would overwrite
; this one's. Clobbers X and Y.
FNBLIT: ldx     FNBGEN
        sta     FNRSEL+FH_BGO
        ldy     #128
FNBL1:  cpx     FNBGEN
        bne     FNBL2
        dey
        bne     FNBL1
FNBL2:  rts

; FNBARG -- blit arguments: A = src low, X = dst low, Y = cnt; both high
; bytes zero.
FNBARG: sta     FNRSEL+FH_BSL
        stx     FNRSEL+FH_BDL
        sty     FNRSEL+FH_BCNT
        lda     #0
        sta     FNRSEL+FH_BSH
        sta     FNRSEL+FH_BDH
        rts

        IF      MHASTXT
; FNWSEL -- select path buffer A (FP_SEL0..3).
FNWSEL: sta     FNRSEL+FH_PATHO
        rts

; FNWRST -- empty the selected buffer.
FNWRST: lda     #FP_RST
        sta     FNRSEL+FH_PATHO
        rts

; FNWRAW -- stream the selected buffer into TX, exactly as it stands.
FNWRAW: lda     #FP_TXRAW
        sta     FNRSEL+FH_PATHO
        rts
        ENDIF

        IF      MHASINP
; ---------------------------------------------------------------------------
; INSCAN -- the stick, the button and SELECT/RESET into INCUR (levels, active
; high); returns (and leaves in MINP) only what is NEWLY pressed.
INSCAN: lda     SWCHA
        eor     #$FF
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        sta     INCUR
        lda     INPT4
        bmi     INSC1
        lda     INCUR
        ora     #IN_FIRE
        sta     INCUR
INSC1:  lda     SWCHB
        lsr     a               ; bit 0 RESET into C
        bcs     INSC2
        tax
        lda     INCUR
        ora     #IN_RST
        sta     INCUR
        txa
INSC2:  lsr     a               ; bit 1 SELECT into C
        bcs     INSC3
        lda     INCUR
        ora     #IN_SEL
        sta     INCUR
INSC3:  lda     INPREV
        eor     #$FF
        and     INCUR
        tax
        lda     INCUR
        sta     INPREV
        stx     MINP
        txa
        rts

; INREPT -- INSCAN plus auto-repeat on the directions only.
INDLAY  EQU     24
INRATE  EQU     6
INREPT: jsr     INSCAN
        beq     INRP1
        ldx     #INDLAY
        stx     INRPT
        rts
INRP1:  lda     INCUR
        and     #INDIRS
        beq     INRP2
        lda     INRPT
        beq     INRPF
        dec     INRPT
        bne     INRP3
INRPF:  lda     #INRATE
        sta     INRPT
        lda     INCUR
        and     #INDIRS
        sta     MINP
        rts
INRP2:  lda     #INDLAY
        sta     INRPT
INRP3:  lda     #0
        sta     MINP
        rts
        ENDIF

        IF      MHASDEC
; ---------------------------------------------------------------------------
; MDEC -- print the 16-bit MNUM/MNUMH as decimal into the row being
; composed, in a field exactly A (1-5) wide, right-aligned. Clobbers X, Y,
; MNUM. (bslib.inc's BSDEC.)
MDEC:   eor     #$FF
        clc
        adc     #6
        asl     a
        tax
        lda     #0
        sta     MDGT
MD1:    lda     MDGT
        and     #$80
        sta     MDGT
MD2:    lda     MNUM
        sec
        sbc     MPOW10,x
        tay
        lda     MNUMH
        sbc     MPOW10+1,x
        bcc     MD3
        sta     MNUMH
        sty     MNUM
        inc     MDGT
        jmp     MD2
MD3:    lda     MDGT
        and     #$0F
        bne     MD4
        cpx     #8
        beq     MD4
        bit     MDGT
        bpl     MD5
MD4:    lda     MDGT
        ora     #$80
        sta     MDGT
        and     #$0F
        clc
        adc     #'0'
        jmp     MD6
MD5:    lda     #' '
MD6:    sta     FNRSEL+FH_TCHR
        inx
        inx
        cpx     #10
        bne     MD1
        rts
MPOW10: DW      10000
        DW      1000
        DW      100
        DW      10
        DW      1

; MDEC8 -- A (0-255) in a field X wide.
MDEC8:  sta     MNUM
        lda     #0
        sta     MNUMH
        txa
        jmp     MDEC
        ENDIF
