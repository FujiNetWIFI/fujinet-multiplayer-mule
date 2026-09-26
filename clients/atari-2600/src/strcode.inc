; strcode.inc -- banks 13 and 14: a message into path buffer 3, and drawn.
;
; Entered from a screen's loop (scrloop.inc) when MSGREQ is set, which is
; only while a state has just arrived: '@' and a player event's amounts read
; the reply window. The message goes into path buffer 3 -- the console could
; never hold it -- with eleven spaces first when it is longer than a row, so
; it scrolls in from the right; the screen's vblank scrolls it (ticker.inc)
; by blitting twelve characters of the buffer at a time. A short one just
; stands. Back to the screen with ENPOST.
;
; The including bank supplies STRTAB (the message's pointer, from its
; number in X) and STRPRE (what goes before it: "*** " for an event).

SENTRY: lda     MSGREQ
        ldx     #0
        stx     MSGREQ
        and     #MSEVENT-1
        tax
        jsr     STRTAB          ; FNPTRL/H: the message; MNUM(2): numbers
        lda     #PBEDIT
        sta     FNRSEL+FH_PATHO
        lda     #FP_RST
        sta     FNRSEL+FH_PATHO
        ldy     #0              ; long (more than a row)?
SE1:    lda     (FNPTRL),y
        beq     SE2
        iny
        bne     SE1
SE2:    cpy     #FNTCOL-3
        bcc     SE4             ; short: no run-in
        ldx     #FNTCOL-1
        lda     #' '
SE3:    sta     FNRSEL+FH_PATHC
        dex
        bne     SE3
SE4:    jsr     STRPRE
        ldy     #0
        sty     SNUMS           ; which number is next
SE5:    lda     (FNPTRL),y
        beq     SE9
        cmp     #'%'
        bne     SE6
        sty     SY
        jsr     SNUM
        ldy     SY
        jmp     SE8
SE6:    cmp     #'@'
        bne     SE7
        sty     SY
        jsr     SNAME
        ldy     SY
        jmp     SE8
SE7:    sta     FNRSEL+FH_PATHC
SE8:    iny
        bne     SE5
SE9:
; drawn at the start; scrolled if longer than a row
        lda     #0
        sta     MSGPOS
        sta     MSGCNT
        lda     FNPLNL
        cmp     #FNTCOL+1
        bcc     SE10
        lda     #TICKRATE
        sta     MSGCNT
SE10:   ldx     MSCR
        lda     SROWS,x
        tax
        lda     #0
        ldy     #FNTCOL
        jsr     FNBARG
        lda     #FB_PATH
        jsr     FNBLIT
        lda     #ENPOST
        sta     MENT
        lda     MSCR
        jmp     MGOTO

; SNUM -- the next number in decimal: MNUM, then SNUM2.
SNUM:   lda     SNUMS
        beq     SN1
        lda     SNUM2
        sta     MNUM
        lda     SNUM2H
        sta     MNUMH
SN1:    inc     SNUMS
        ldx     #0
        stx     MDGT
SN2:    ldy     #0
SN3:    lda     MNUM
        sec
        sbc     SPOWL,x
        sta     MTMP
        lda     MNUMH
        sbc     SPOWH,x
        bcc     SN4
        sta     MNUMH
        lda     MTMP
        sta     MNUM
        iny
        bne     SN3
SN4:    tya
        bne     SN5
        bit     MDGT
        bmi     SN5
        cpx     #4
        bne     SN6
SN5:    lda     #$80
        sta     MDGT
        tya
        ora     #'0'
        sta     FNRSEL+FH_PATHC
SN6:    inx
        cpx     #5
        bne     SN2
        rts
SPOWL:  DB      (10000)&$FF,(1000)&$FF,100,10,1
SPOWH:  DB      (10000)>>8,(1000)>>8,0,0,0

; SNAME -- the name of the player whose turn it is (turn.seat).
SNAME:  ldx     FNRPLY+STSEAT
        cpx     #4
        bcs     SNM9
        lda     SPLOFS,x
        tax
        ldy     #8
SNM1:   lda     FNRPLY+SPLAYER+PLNAME,x
        beq     SNM9
        sta     FNRSEL+FH_PATHC
        inx
        dey
        bne     SNM1
SNM9:   rts

SPLOFS: DB      0,PLSIZE,2*PLSIZE,3*PLSIZE
; the message row, by the screen's bank (mdefs.inc BK*)
SROWS:  DB      1,1,1,20,1,1,1,19,1,15,1,1,1,1,1,1
