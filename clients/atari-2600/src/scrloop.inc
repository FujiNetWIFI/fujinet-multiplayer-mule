; scrloop.inc -- the frame loop every screen bank runs.
;
; One frame (the bank's kernel: DFRAME, whose hook APPVBL runs in the
; vblank), then the overscan's work, which has 2240 cycles:
;   * a message to make (banks 13/14) or a text row to draw (bank 10) goes
;     first, on a fresh overscan -- each is most of one -- and the frame
;     ends there;
;   * else the bank's own work (STICK: a helper bank comes back to SLOOP1
;     with ENPOST);
;   * then, if a request is in flight, queued or due, and the overscan has
;     room left (the RIOT timer says), one step of the request machine in
;     bank 2, which comes back with ENRUN (or ENSTATE / ENENTER).
;
; No request starts while HFNEW is set: the screen is still reading the last
; state out of the reply window, and the next transaction would repaint it.

        IFNDEF  MHASMSG
MHASMSG EQU     0
        ENDIF
NETMIN  EQU     18              ; 64-cycle units a request step needs

; SLOOPF -- into the loop from a screen change: arm the overscan timer
; first (a stale one could be seconds).
SLOOPF: lda     #OSTIM
        sta     TIM64T
        lda     #0
        sta     VBLANK
        jmp     SLOOP
SLOOP:  jsr     DFRAME
        IF      MHASMSG
        lda     MSGREQ          ; a message to make: banks 13/14 (the reply
        beq     SLOOP4          ;   window is whole: a state just arrived)
        ldx     #BKSTRM
        cmp     #MSEVENT
        bcc     SLOOPM
        ldx     #BKSTRP
SLOOPM: txa
        jmp     MGOTO
SLOOP4:
        ENDIF
        IF      MHASCMP
        lda     MCOMP           ; rows to draw: the compose bank does them
        beq     SLOOP3
        lda     #BKCOMP
        jmp     MGOTO
SLOOP3:
        ENDIF
        jsr     STICK
SLOOP1: lda     HFLAGS
        and     #HFNEW
        beq     SLOOP2
; A state is still being drawn. NTRY (the request machine's step timeout,
; idle between requests; bank 2 sets it at an arrival) counts the frames:
; two seconds and the screen has stalled -- let the next request go anyway,
; rather than freeze on the last state.
        dec     NTRY
        bne     SLOOP
        lda     HFLAGS
        and     #~HFNEW&$FF
        sta     HFLAGS
        lda     #0
        sta     MCOMP
SLOOP2: lda     NSTATE
        bne     SHOP
        lda     NACT
        bne     SHOP
        lda     SNOPOLL
        bne     SLOOP
        lda     HFLAGS
        and     #HFTBL          ; no table joined: nothing to poll
        beq     SLOOP
        dec     NPOLL
        bne     SLOOP
        inc     NPOLL           ; (due: stays due until it can go)
SHOP:   lda     INTIM           ; room for a step? else the next frame's
        cmp     #NETMIN
        bcc     SLOOP
        lda     #BKNET
        jmp     MGOTO

; QACT -- queue request A with argument X (and Y: the high byte). It rides
; the next visit to bank 2; a request already in flight finishes first.
QACT:   sta     NACT
        stx     NACTA
        sty     NACTH
        rts

; RSTCHK -- RESET held for a second leaves the table: /leave, and the seat
; goes to the computer. From APPVBL, after INSCAN.
RSTCHK: lda     INCUR
        and     #IN_RST
        bne     RSC1
        sta     MHOLD
        rts
RSC1:   inc     MHOLD
        lda     MHOLD
        cmp     #60
        bne     RSC2
        lda     #RQLEAVE
        ldx     #0
        ldy     #0
        jmp     QACT
RSC2:   rts
