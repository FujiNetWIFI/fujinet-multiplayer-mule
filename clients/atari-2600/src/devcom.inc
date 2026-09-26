; devcom.inc -- what banks 11 (the map's walker) and 12 (the town's) both
; do: a turn's numbers, the answer to an action, the time, a step's start,
; the players' pointers, a request queued. CODE.

; TURNX -- the turn's numbers, for either screen: time, my species and
; colour, what I lead, the wampus.
TURNX:  lda     FNRPLY+STPTU
        bne     TX1
        lda     #28
TX1:    sta     DVPTU
        lda     FNRPLY+STLEFT
        sta     DVLEFT
        lda     FNRPLY+STLEFT+1
        sta     DVLEFTH
        lda     FNRPLY+STELAP
        sta     DVEL
        lda     FNRPLY+STELAP+1
        sta     DVELH
        ldx     HSEAT
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLSPEC,x
        and     #7
        sta     WSP
        ldx     HSEAT
        lda     SEATCOL,x
        sta     WCOL
        sta     FIGCOL
        sta     BARCOL
        lda     FNRPLY+STFLAGS  ; the server says what we lead
        and     #TF_MULE
        beq     TX2
        lda     #1
TX2:    sta     WMULE
; DWAMP -- the wampus I will meet: the entry of turn.wampus showing now, or
; the next to show. (Only sent to the player whose turn it is.)
        lda     #$FF
        sta     WPPLOT
        jsr     DELAPSED        ; MTMP/MTMP2: the turn's jiffies now
        ldx     FNRPLY+STNWAMP
        cpx     #8              ; entries past 7 run into the second slice
        bcc     DW1
        ldx     #7
DW1:    dex
        bmi     DW9
        txa                     ; entry * 6
        asl     a
        sta     MNUM
        asl     a
        clc
        adc     MNUM
        tay
        lda     FNRPLY+STWAMP+4,y       ; to >= now
        cmp     MTMP
        lda     FNRPLY+STWAMP+5,y
        sbc     MTMP2
        bcc     DW1
        lda     FNRPLY+STWAMP+0,y       ; the earliest such: the next to come
        sta     WPPLOT
        lda     FNRPLY+STWAMP+1,y
        sta     WPMNT
        lda     FNRPLY+STWAMP+2,y
        sta     WPFROM
        lda     FNRPLY+STWAMP+3,y
        sta     WPFROMH
        lda     FNRPLY+STWAMP+4,y
        sta     WPTO
        lda     FNRPLY+STWAMP+5,y
        sta     WPTOH
        jmp     DW1
DW9:    rts

; ACTRES -- what the server said about the action sent (DVACT), as a
; message, once the state is its answer (NREQ, the request that fetched it).
; Errors first. Z set: not answered yet.
ACTRES: lda     DVACT
        beq     AR9
        cmp     NREQ            ; not until the state is that request's
        bne     AR9             ;   answer: a poll in flight may land first
        ldx     #0
        stx     DVACT
        ldx     FNRPLY+SHERR
        beq     AR2
        ldy     #MSGMONEY
        cpx     #ERR_MONEY
        beq     AR8
        ldy     #MSGNOMULE
        cpx     #ERR_NOSTOCK
        beq     AR8
        ldy     #MSGNO
        bne     AR8
AR2:    tax                     ; the action
        lda     FNRPLY+SHRES
        ldy     #MSGRAN         ; install: 1 = it ran off
        cpx     #RQINST
        bne     AR3
        cmp     #1
        beq     AR8
        ldy     #MSGINST
        bne     AR8
AR3:    ldy     #MSGCAUGHT
        cpx     #RQWAMP
        beq     AR8
        ldy     #MSGSALE
        cpx     #RQSELL
        beq     AR8
        ldy     #MSGUNSALE
        cpx     #RQUNSEL
        beq     AR8
        cpx     #RQASSAY
        bne     AR4
        and     #7              ; the assay: the result's level
        cmp     #5
        bcc     AR3A
        lda     #4
AR3A:   clc
        adc     #MSGASSAY
        tay
        lda     #$FF            ; used up
        sta     DVSMP
        bne     AR8
AR4:    ldy     #MSGPUB
        cpx     #RQPUB
        bne     AR4A
        lda     FNRPLY+SHRES    ; the winnings, kept: the row is drawn after
        sta     DVSMP           ;   later polls (DVSMP/DVLIST are dead once
        lda     FNRPLY+SHRES+1  ;   the pub has ended the turn)
        sta     DVLIST
        jmp     AR8
AR4A:
        ldy     #MSGNONE        ; bought, returned, outfitted: nothing to say
AR8:    sty     DMSG
AR9:    rts

; DELAPSED -- MTMP/MTMP2 = the turn's elapsed jiffies now.
DELAPSED: lda   MFRAME
        sec
        sbc     HRXF
        sta     MTMP
        lda     MFRAMEH
        sbc     HRXFH
        sta     MTMP2
        lda     MTMP
        clc
        adc     DVEL
        sta     MTMP
        lda     MTMP2
        adc     DVELH
        sta     MTMP2
        rts

; SETFACE -- the facing A, keeping the walk's toggle (bit 7).
SETFACE: sta    MTMP
        lda     WFACE
        and     #$80
        ora     MTMP
        sta     WFACE
        rts

; STEPX -- a step starts: the old place for the M.U.L.E., the walk's toggle,
; the next step's wait.
STEPX:  lda     WX
        sta     WTX
        lda     WY
        sta     WTY
        lda     #5              ; (a terrain's extra is added after)
        sta     WCNT
        lda     WFACE
        eor     #$80            ; bit 7: the walk's toggle
        sta     WFACE
        rts

; P0SET / P1SET -- the player's pointer: the shape at X:A, less its top.
P0SET:  sec
        sbc     P0TOP
        sta     P0PTR
        txa
        sbc     #0
        sta     P0PTRH
        rts
P1SET:  sec
        sbc     P1TOP
        sta     P1PTR
        txa
        sbc     #0
        sta     P1PTRH
        rts

; QACT -- queue request A with argument X (and Y). (scrloop.inc's.)
QACT:   sta     NACT
        stx     NACTA
        sty     NACTH
        rts

; XBACK -- back to the screen, which carries on after its STICK.
XBACK:  lda     #ENPOST
        sta     MENT
        lda     MSCR
        jmp     MGOTO

PLOFS:  DB      0,PLSIZE,2*PLSIZE,3*PLSIZE
SEATCOL: DB     CPLAY0,CPLAY1,CPLAY2,CPLAY3
