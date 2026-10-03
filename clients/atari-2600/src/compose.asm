; compose.asm -- bank 10: text rows, for the screen banks that have no
; room for their own.
;
; A screen with rows to draw (MCOMP nonzero) comes here in its overscan, one
; row a visit, and is sent straight back (ENRUN): the same shape as a visit
; to the request machine. Which screen is asking is MSCR. The rows read the
; reply window, so they are only started when a state arrives and no request
; begins until they are done (HFNEW stays set until the last).

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     0
MHASDEC EQU     1

        INCLUDE "../build/tail.inc"
        INCLUDE "aucram.inc"
        INCLUDE "devram.inc"

ARB0    EQU     17              ; the auction's first text row under the arena
MBOTROW EQU     17              ; the development screens' row under the map

        ORG     $1000

CENTRY: lda     MSCR
        cmp     #BKAUC
        bne     CE0
        jsr     ACOMPOSE
        jmp     CE1
CE0:    cmp     #BKDEV
        beq     CE0A
        cmp     #BKTOWN
        bne     CE1
CE0A:   jsr     DCOMPOSE
CE1:    lda     #ENRUN
        sta     MENT
        lda     MSCR
        jmp     MGOTO

; ---------------------------------------------------------------------------
; ACOMPOSE -- the auction's rows, one a visit.
ACOMPOSE: lda   MCOMP
        bne     ACC0
        rts
ACC0:   inc     MCOMP
        cmp     #1
        bne     ACC2
        lda     #0              ; the title
        jsr     FNROWA
        lda     AKIND
        cmp     #1
        bne     ACC1
        ldx     #(SLAND)&$FF
        ldy     #(SLAND)>>8
        jsr     FNSTRXY
        jmp     FNENDW
ACC1:   ldx     #(SAUCN)&$FF
        ldy     #(SAUCN)>>8
        jsr     FNSTRXY
        lda     HROUND
        ldx     #2
        jsr     MDEC8
        jmp     FNENDW
ACC2:   cmp     #2
        JNE     ACC3
        lda     #1              ; the good, or the land's price
        jsr     FNROWA
        lda     AKIND
        cmp     #1
        beq     ACC2L
        lda     #' '
        jsr     FNCHR
        lda     #' '
        jsr     FNCHR
        ldx     AGOOD
        lda     GNAMEL,x
        ldy     GNAMEH,x
        tax
        jsr     FNSTRXY
        jmp     FNENDW
; The land's price row (MekkoGX au_land_line): the opening price; the high
; bid once a buyer's line passes it; what it sold for.
ACC2L:  lda     FNRPLY+SASTART
        sta     MNUM
        lda     FNRPLY+SASTART+1
        sta     MNUMH
        ldx     #0              ; MIDX: 0 start, 1 the high bid, 2 sold
        stx     MIDX
        lda     ASTG
        cmp     #STGDONE
        bne     ACC2B
        lda     FNRPLY+SAPRICE  ; sold (a price of 0: nobody bid)
        ora     FNRPLY+SAPRICE+1
        beq     ACC2W
        lda     FNRPLY+SAPRICE
        sta     MNUM
        lda     FNRPLY+SAPRICE+1
        sta     MNUMH
        lda     #2
        sta     MIDX
        bne     ACC2W
ACC2B:  lda     #3              ; the buyers: the highest line at or over
        sta     MTMP2           ;   the start
ACC2C:  ldy     MTMP2
        lda     FNRPLY+SAROLE,y
        cmp     #ROLEBUY
        bne     ACC2D
        tya
        asl     a
        tay
        lda     FNRPLY+SAPOS,y
        cmp     MNUM
        lda     FNRPLY+SAPOS+1,y
        sbc     MNUMH
        bcc     ACC2D
        lda     FNRPLY+SAPOS,y
        sta     MNUM
        lda     FNRPLY+SAPOS+1,y
        sta     MNUMH
        lda     #1
        sta     MIDX
ACC2D:  dec     MTMP2
        bpl     ACC2C
ACC2W:  ldx     MIDX
        lda     LPRL,x
        ldy     LPRH,x
        tax
        jsr     FNSTRXY
        lda     #5
        jsr     MDEC
        jmp     FNENDW
ACC3:   cmp     #3
        bne     ACC4
        lda     #ARB0           ; me: my role and my line's price
        jsr     FNROWA
        ldx     HSEAT
        cpx     #4
        bcs     ACC3S
        lda     AROLE,x
        tax
        lda     ROLEL,x
        ldy     ROLEH,x
        tax
        jsr     FNSTRXY
        ldx     HSEAT
        lda     AROLE,x
        beq     ACC3X
        lda     #' '
        jsr     FNCHR
        lda     #'$'
        jsr     FNCHR
        lda     HSEAT
        asl     a
        tax
        lda     FNRPLY+SAPOS,x
        sta     MNUM
        lda     FNRPLY+SAPOS+1,x
        sta     MNUMH
        lda     #5
        jsr     MDEC
ACC3X:  jmp     FNENDW
ACC3S:  ldx     #(SSPECT)&$FF   ; no seat: we only watch
        ldy     #(SSPECT)>>8
        jsr     FNSTRXY
        jmp     FNENDW
ACC4:   cmp     #4
        bne     ACC5
        lda     #ARB0+1         ; my money, my units of the good
        jsr     FNROWA
        ldx     HSEAT
        cpx     #4
        bcs     ACC4X
        lda     #'$'
        jsr     FNCHR
        lda     HMONEY
        sta     MNUM
        lda     HMONEYH
        sta     MNUMH
        lda     #5
        jsr     MDEC            ; "$ 1000" -- then " HAS  12": 12 columns
        lda     AKIND
        cmp     #2
        bne     ACC4X
        ldx     HSEAT
        cpx     #4
        bcs     ACC4X
        ldx     #(SUNITS)&$FF
        ldy     #(SUNITS)>>8
        jsr     FNSTRXY
        ldx     HSEAT
        lda     PLOFS,x
        clc
        adc     AGOOD
        tax
        lda     FNRPLY+SPLAYER+PLGOODS,x
        ldx     #2
        jsr     MDEC8
ACC4X:  jmp     FNENDW
ACC5:   lda     #ARB0+2         ; the message, then done
        jsr     FNROWA
        lda     ASTG
        cmp     #STGDONE
        bne     ACC5A
        ldx     #(SOVER)&$FF
        ldy     #(SOVER)>>8
        lda     AKIND
        cmp     #1
        bne     ACC5W
        ldx     #(SSOLD)&$FF
        ldy     #(SSOLD)>>8
        lda     FNRPLY+SAWIN
        bpl     ACC5W
        ldx     #(SNOBID)&$FF
        ldy     #(SNOBID)>>8
        jmp     ACC5W
ACC5A:  cmp     #STGACT
        bne     ACC5Z
        lda     FNRPLY+SAFLAGS
        and     #AF_CRIT
        beq     ACC5C
        ldx     #(SCRIT)&$FF
        ldy     #(SCRIT)>>8
        jmp     ACC5W
ACC5C:  lda     FNRPLY+SAFLAGS
        and     #AF_COLL
        beq     ACC5B
        ldx     #(SCOLL)&$FF
        ldy     #(SCOLL)>>8
        jmp     ACC5W
ACC5B:  lda     AKIND
        cmp     #2
        bne     ACC5Z
        ldx     #(STRADED)&$FF
        ldy     #(STRADED)>>8
        jsr     FNSTRXY
        lda     ATRADED
        ldx     #3
        jsr     MDEC8
        jmp     ACC5Z
ACC5W:  jsr     FNSTRXY
ACC5Z:  jsr     FNENDW
        lda     #0
        sta     MCOMP
        lda     HFLAGS
        and     #~HFNEW&$FF
        sta     HFLAGS
        rts


; ---------------------------------------------------------------------------
; DCOMPOSE -- development (the map, bank 5, and the town, bank 6): the
; title, the message, my money.
DCOMPOSE: lda   MCOMP
        inc     MCOMP
        cmp     #1
        bne     DC2
        lda     #0
        jsr     FNROWA
        ldx     #(SDEVT)&$FF
        ldy     #(SDEVT)>>8
        jsr     FNSTRXY
        lda     HROUND
        ldx     #2
        jsr     MDEC8
        jmp     FNENDW
DC2:    cmp     #2
        bne     DC3
        lda     MSGCNT          ; a message scrolls there: leave it
        bne     DC2Z
        lda     #1
        jsr     FNROWA
        ldx     DMSG
        lda     DVST
        cmp     #DVPROMPT
        bne     DC2A
        ldx     #MSGSTART
DC2A:   cpx     #MSGPUB+1
        bcc     DC2B
        ldx     #MSGNONE
DC2B:   txa
        pha
        lda     DMSGLO,x
        ldy     DMSGHI,x
        tax
        jsr     FNSTRXY
        pla
        cmp     #MSGPUB         ; the pub: what it paid (the arrival's result)
        bne     DC2C
        lda     DVSMP           ; the winnings (devcom.inc ACTRES)
        sta     MNUM
        lda     DVLIST
        sta     MNUMH
        lda     #4
        jsr     MDEC
DC2C:   jmp     FNENDW
DC2Z:   rts
DC3:    lda     #MBOTROW
        jsr     FNROWA
        lda     #'$'
        jsr     FNCHR
        lda     HMONEY
        sta     MNUM
        lda     HMONEYH
        sta     MNUMH
        lda     #5
        jsr     MDEC
        jsr     FNENDW
        lda     #0
        sta     MCOMP
        lda     HFLAGS
        and     #~HFNEW&$FF
        sta     HFLAGS
        rts

SDEVT:  DB      "DEVELOP #",0
DM0:    DB      " ",0
DM1:    DB      "FIRE: START",0
DM2:    DB      "IT RAN AWAY",0
DM3:    DB      "INSTALLED",0
DM4:    DB      "CAUGHT IT!",0
DM5:    DB      "PICK A PLOT",0
DM6:    DB      "SAMPLE TAKEN",0
DM7:    DB      "CAN'T",0
DM8:    DB      "NO MONEY",0
DM9:    DB      "NO M.U.L.E.S",0
DM10:   DB      "NO SAMPLE",0
DM11:   DB      "FOR SALE",0
DM12:   DB      "WITHDRAWN",0
DM13:   DB      "NO CRYSTITE",0
DM14:   DB      "LOW CRYSTITE",0
DM15:   DB      "MEDIUM",0
DM16:   DB      "HIGH",0
DM17:   DB      "VERY HIGH",0
DM18:   DB      "WON $",0

PLOFS:  DB      0,PLSIZE,2*PLSIZE,3*PLSIZE

SLAND:  DB      "LAND AUCTION",0
SAUCN:  DB      "AUCTION #",0
SSTART: DB      "START $",0
SHIGH:  DB      "BID   $",0
SSOLDP: DB      "SOLD  $",0
SUNITS: DB      " HAS",0
SOVER:  DB      "AUCTION OVER",0
SSOLD:  DB      "PLOT IS SOLD",0
SNOBID: DB      "NO BIDS",0
SCOLL:  DB      "COLLUSION",0
SCRIT:  DB      "CRITICAL LVL",0
STRADED: DB     "TRADED",0
GN0S:   DB      "FOOD",0
GN1S:   DB      "ENERGY",0
GN2S:   DB      "SMITHORE",0
GN3S:   DB      "CRYSTITE",0
RL0:    DB      "WATCHING",0
RL1:    DB      "BUY",0
RL2:    DB      "SELL",0
SSPECT: DB      " SPECTATOR",0


        INCLUDE "mlib.inc"

; ---------------- data: tables, not code (tools/dataranges.py) ----------
DSTART:
GNAMEL: DB      (GN0S)&$FF,(GN1S)&$FF,(GN2S)&$FF,(GN3S)&$FF
GNAMEH: DB      (GN0S)>>8,(GN1S)>>8,(GN2S)>>8,(GN3S)>>8
ROLEL:  DB      (RL0)&$FF,(RL1)&$FF,(RL2)&$FF
ROLEH:  DB      (RL0)>>8,(RL1)>>8,(RL2)>>8
LPRL:   DB      (SSTART)&$FF,(SHIGH)&$FF,(SSOLDP)&$FF
LPRH:   DB      (SSTART)>>8,(SHIGH)>>8,(SSOLDP)>>8
DMSGLO: DB      (DM0)&$FF,(DM1)&$FF,(DM2)&$FF,(DM3)&$FF,(DM4)&$FF,(DM5)&$FF,(DM6)&$FF
        DB      (DM7)&$FF,(DM8)&$FF,(DM9)&$FF,(DM10)&$FF,(DM11)&$FF,(DM12)&$FF
        DB      (DM13)&$FF,(DM14)&$FF,(DM15)&$FF,(DM16)&$FF,(DM17)&$FF,(DM18)&$FF
DMSGHI: DB      (DM0)>>8,(DM1)>>8,(DM2)>>8,(DM3)>>8,(DM4)>>8,(DM5)>>8,(DM6)>>8
        DB      (DM7)>>8,(DM8)>>8,(DM9)>>8,(DM10)>>8,(DM11)>>8,(DM12)>>8
        DB      (DM13)>>8,(DM14)>>8,(DM15)>>8,(DM16)>>8,(DM17)>>8,(DM18)>>8

        END
