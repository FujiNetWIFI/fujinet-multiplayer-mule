; ticker.inc -- a message longer than a row scrolls through it, once
; (banks 13/14 put it in path buffer 3 and draw its start). From the vblank.
; The including bank says which row: TICKROW.

TICKER: lda     MSGCNT
        beq     TK9
        dec     MSGCNT
        bne     TK9
        lda     #TICKRATE
        sta     MSGCNT
        lda     #PBEDIT         ; (selecting publishes its length)
        sta     FNRSEL+FH_PATHO
        inc     MSGPOS
        lda     MSGPOS
        cmp     FNPLNL
        bcc     TK1
        lda     #0              ; through once: done
        sta     MSGCNT
TK1:    lda     MSGPOS
        ldx     #TICKROW
        ldy     #FNTCOL
        jsr     FNBARG
        lda     #FB_PATH
        jmp     FNBLIT
TK9:    rts
