; url.inc -- the devicespec, streamed into the TX page a character at a
; time. Nothing is assembled in RAM: the endpoint, the table and the name
; are cartridge path buffers, streamed with FP_TXRAW; the route is ROM; the
; one number is printed as it goes.
;
;   N:https://mule.fujinet.online/turn/install/31?table=irata&player=BOB&bin=1

; UPUT -- append the NUL-terminated string at X (lo) / Y (hi) to TX.
UPUT:   stx     FNPTRL
        sty     FNPTRH
        ldy     #0
UPT1:   lda     (FNPTRL),y
        beq     UPT2
        sta     FNTX
        iny
        bne     UPT1
UPT2:   rts

; UDEC -- append A (0-255) as decimal, no leading zeros. Clobbers X.
UDEC:   ldx     #0
UDC1:   cmp     #100
        bcc     UDC2
        sbc     #100
        inx
        bne     UDC1
UDC2:   pha
        txa
        beq     UDC3            ; no hundreds
        ora     #'0'
        sta     FNTX
        pla
        ldx     #'0'
        bne     UDC4            ; always: tens print after hundreds
UDC3:   pla
        ldx     #0
UDC4:   stx     MTMP            ; '0' if tens must print, 0 if not
        ldx     #0
UDC5:   cmp     #10
        bcc     UDC6
        sbc     #10
        inx
        bne     UDC5
UDC6:   pha
        txa
        bne     UDC7
        lda     MTMP
        beq     UDC8
        txa
UDC7:   ora     #'0'
        sta     FNTX
UDC8:   pla
        ora     #'0'
        sta     FNTX
        rts

; UDEC16 -- append NARGH:NARG (0-25599) as decimal. Clobbers X, Y, MNUM.
UDEC16: lda     NARGH
        bne     UD161
        lda     NARG
        jmp     UDEC
UD161:  sta     MNUMH
        lda     NARG
        ldx     #0              ; the hundreds
UD162:  sec
        sbc     #100
        tay
        lda     MNUMH
        sbc     #0
        bcc     UD163
        sta     MNUMH
        tya
        inx
        bne     UD162
UD163:  tya
        clc
        adc     #100            ; what is left, 0-99
        sta     MNUM
        txa
        jsr     UDEC
        lda     MNUM            ; two digits
        ldx     #'0'
UD164:  cmp     #10
        bcc     UD165
        sbc     #10
        inx
        bne     UD164
UD165:  stx     FNTX
        ora     #'0'
        sta     FNTX
        rts

; BLDURL -- the devicespec for NREQ (and NARG / NARGH).
BLDURL: lda     #'N'
        sta     FNTX
        lda     #':'
        sta     FNTX
        lda     #PBEND
        sta     FNRSEL+FH_PATHO
        lda     #FP_TXRAW
        sta     FNRSEL+FH_PATHO
        ldx     NREQ
        lda     RTLO,x
        ldy     RTHI,x
        tax
        jsr     UPUT
        ldx     NREQ
        cpx     #RQTABLE
        bne     BU1
        rts                     ; /tables carries its own query
BU1:    lda     RTARG,x
        beq     BUQ
        cmp     #1
        bne     BU2
        lda     NARG            ; a number
        jsr     UDEC
        jmp     BUQ
BU2:    cmp     #2
        bne     BU2A
        ldx     NARG            ; /auction/push/WORD/UNITS
        lda     PWLO,x
        ldy     PWHI,x
        tax
        jsr     UPUT
        lda     #'/'
        sta     FNTX
        lda     NARGH
        jsr     UDEC
        jmp     BUQ
BU2A:   cmp     #4
        bne     BU3
        jsr     UDEC16          ; /auction/push/hold/0/PRICE
        jmp     BUQ
BU3:    ldx     NARG            ; /declare/n|b|s
        lda     DWORD,x
        sta     FNTX
BUQ:    rts

; BLDURL2 -- the query: the table, the player, the format.
BLDURL2: ldx    NREQ
        cpx     #RQTABLE
        bne     BU9
        rts
BU9:    ldx     #(UQTAB)&$FF
        ldy     #(UQTAB)>>8
        jsr     UPUT
        lda     #PBTABLE
        sta     FNRSEL+FH_PATHO
        lda     #FP_TXRAW
        sta     FNRSEL+FH_PATHO
        ldx     #(UQPLR)&$FF
        ldy     #(UQPLR)>>8
        jsr     UPUT
        lda     #PBNAME
        sta     FNRSEL+FH_PATHO
        lda     #FP_TXRAW
        sta     FNRSEL+FH_PATHO
        ldx     #(UQBIN)&$FF
        ldy     #(UQBIN)>>8
        jmp     UPUT

; The routes, by RQ*.
R01:    DB      "/state",0
R02:    DB      "/ready/1",0
R03:    DB      "/ready/0",0
R04:    DB      "/setup/species/",0
R05:    DB      "/claim/",0
R06:    DB      "/auction/push/",0
R07:    DB      "/declare/",0
R08:    DB      "/collude",0
R09:    DB      "/turn/begin",0
R10:    DB      "/turn/buymule",0
R11:    DB      "/turn/outfit/",0
R12:    DB      "/turn/install/",0
R13:    DB      "/turn/return",0
R14:    DB      "/turn/assay/",0
R15:    DB      "/turn/sell/",0
R16:    DB      "/turn/unsell/",0
R17:    DB      "/turn/wampus",0
R18:    DB      "/turn/pub",0
R19:    DB      "/turn/end",0
R20:    DB      "/leave",0
R21:    DB      "/tables?bin=1",0
; the units seen don't matter to a release
R22:    DB      "/auction/push/hold/0/",0
RTARG:  DB      0,0,0,0,1,1,2,3,0,0,0,1,1,0,1,1,1,0,0,0,0,0,4
PW0:    DB      "hold",0
PW1:    DB      "up",0
PW2:    DB      "down",0
DWORD:  DB      "nbs"
UQTAB:  DB      "?table=",0
UQPLR:  DB      "&player=",0
UQBIN:  DB      "&bin=1",0
