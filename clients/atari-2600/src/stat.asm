; stat.asm -- bank 3: the status summary (phase 7) and game over (phase 8).
;
; MekkoGX status.c, as text. Each seat is a band of four rows in its colour:
; its rank and name (and OK once it has pressed), then money, land and the
; total. Land is the server's ScoreOf: 500 a plot, and for a plot with a
; M.U.L.E. the outfit (25 a good's step) plus 35. Under the bands, the
; colony's total -- which is in the reply's second slice, bytes 512-549.
;
; The summary waits for every human's button (/ready). The Atari draws it
; for 410 jiffies first, so FIRE is taken from then on; the bell says so.
; Game over names the first founder and the colony's rating.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     1
MHASCMP EQU     0
MHASMSG EQU     1
TICKROW EQU     20

        INCLUDE "../build/tail.inc"

SPRESS  EQU     PV+0            ; I have pressed
SBELL   EQU     PV+1            ; the bell has rung
SLAND   EQU     PV+2            ; the land value being summed, 16 bits
SLANDH  EQU     PV+3
STOT0   EQU     PV+4            ; the colony total, 24 bits (of 32)
STOT1   EQU     PV+5
STOT2   EQU     PV+6
SRATE   EQU     PV+7            ; the colony's rating
SWIN    EQU     PV+8            ; the seat ranked first
SRDY    EQU     PV+9            ; my PF_READY
SEL0    EQU     PV+10           ; elapsed jiffies at the arrival, 16 bits
SEL1    EQU     PV+11
SMSGK   EQU     PV+12           ; the message said: 1 press, 2 the rating

DRAWN   EQU     410             ; jiffies the Atari spends drawing

        ORG     $1000

SENTRY: lda     MENT
        cmp     #ENRUN
        bne     SE0
        jmp     SLOOP
SE0:    cmp     #ENPOST
        bne     SE1
        jmp     SLOOP1
SE1:    cmp     #ENSTATE
        bne     SSETUP
        jmp     SSTATE

; ---------------------------------------------------------------------------
SSETUP: lda     #BKSTAT
        sta     MSCR
        lda     #2
        sta     VBLANK
        jsr     DINIT
        jsr     FNCLS
        lda     #$FF
        sta     MBGMSK
        sta     MBAR
        lda     #0
        sta     SNOPOLL
        sta     SPRESS
        sta     SBELL
        sta     SMSGK
        sta     MSGCNT
SSTATE: lda     #1
        sta     MCOMP
        lda     SMSGK           ; the press message was wanted: now
        cmp     #1
        bne     SST9
        lda     #3
        sta     SMSGK
        lda     #MSPRESS
        sta     MSGREQ
SST9:
; The colony block, from slice 1, now -- before anything else reads the
; window, and back to slice 0 after.
        lda     #1
        jsr     SLICE
        lda     FNRPLY+SCOLONY+6-512
        sta     STOT0
        lda     FNRPLY+SCOLONY+7-512
        sta     STOT1
        lda     FNRPLY+SCOLONY+8-512
        sta     STOT2
        lda     FNRPLY+SCOLONY+2-512
        sta     SRATE
        lda     #0
        jsr     SLICE
        lda     HELAP
        sta     SEL0
        lda     HELAPH
        sta     SEL1
        lda     HPHASE          ; game over: the colony's rating, in words
        cmp     #PHOVER
        bne     SST0
        lda     SMSGK
        cmp     #2
        beq     SST0
        lda     #2
        sta     SMSGK
        lda     SRATE
        cmp     #7
        bcs     SST0
        adc     #MSRATE
        sta     MSGREQ
SST0:   lda     MENT            ; a screen change starts the frame afresh;
        cmp     #ENSTATE        ;   a state inside a frame's overscan does not
        beq     SST1
        jmp     SLOOPF
SST1:   jmp     SLOOP

; SLICE -- show reply slice A, and wait (bounded) for the cart to say so.
SLICE:  pha
        ldx     #FR_RXSL
        jsr     FNRW
        pla
        ldy     #0
SLC1:   cmp     FNSECH
        beq     SLC2
        dey
        bne     SLC1
SLC2:   rts

; ---------------------------------------------------------------------------
; SCOMPOSE -- one row of the redraw a frame, from the vblank (a row and a
; number are most of the vblank's 2800 cycles): steps 1-16 are the seats'
; four rows each, then the title, the colony, the footer. HFNEW is cleared
; at the end.
SCOMPOSE: lda   MCOMP
        bne     SC0
        rts
SC0:    inc     MCOMP
        cmp     #17
        bcc     SC1
        bne     SC0A
        jmp     SCTITLE
SC0A:   cmp     #18
        bne     SC0B
        jmp     SCCOL
SC0B:   jmp     SCLAST
SC1:    sec
        sbc     #1
        tay
        and     #3
        sta     MTMP2           ; the row within the band
        tya
        lsr     a
        lsr     a
        sta     MIDX            ; the seat
        tax
        lda     SEATROW,x
        clc
        adc     MTMP2
        jsr     FNROWA
        lda     MTMP2
        beq     SCNAME
        cmp     #1
        beq     SCMONY
        cmp     #2
        beq     SCLAND
        jmp     SCTOT

SCNAME: ldx     MIDX
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLRANK,x
        and     #7
        ora     #'0'
        jsr     FNCHR
        lda     #' '
        jsr     FNCHR
        ldy     #8
SC2:    lda     FNRPLY+SPLAYER+PLNAME,x
        beq     SC3
        jsr     FNCHR
        inx
        dey
        bne     SC2
SC3:    ldx     MIDX
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLFLAGS,x
        and     #PF_READY
        beq     SC4
        lda     HPHASE
        cmp     #PHSUMM
        bne     SC4
        tya                     ; pad the name to 8, then "OK"
        beq     SC3A
        jsr     FNSPC
SC3A:   lda     #'O'
        jsr     FNCHR
        lda     #'K'
        jsr     FNCHR
SC4:    jmp     FNENDW

SCMONY: ldx     #(SMONEY)&$FF
        ldy     #(SMONEY)>>8
        jsr     FNSTRXY
        ldx     MIDX
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLMONEY,x
        sta     MNUM
        lda     FNRPLY+SPLAYER+PLMONEY+1,x
        jmp     SCNUM

; land: 500 a plot, and (good+1)*25+35 for a M.U.L.E. on it
SCLAND: lda     #0
        sta     SLAND
        sta     SLANDH
        ldx     #NPLOTS-1
SL1:    lda     FNRPLY+SMOWNER,x
        cmp     MIDX
        bne     SL3
        lda     SLAND
        clc
        adc     #(500)&$FF
        sta     SLAND
        lda     SLANDH
        adc     #(500)>>8
        sta     SLANDH
        ldy     FNRPLY+SMMULE,x
        bmi     SL3
        cpy     #4
        bcs     SL3
        lda     MULEV,y
        clc
        adc     SLAND
        sta     SLAND
        bcc     SL3
        inc     SLANDH
SL3:    dex
        bpl     SL1
        ldx     #(SLANDS)&$FF
        ldy     #(SLANDS)>>8
        jsr     FNSTRXY
        lda     SLAND
        sta     MNUM
        lda     SLANDH
        jmp     SCNUM

SCTOT:  ldx     #(STOTAL)&$FF
        ldy     #(STOTAL)>>8
        jsr     FNSTRXY
        ldx     MIDX
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLSCORE,x
        sta     MNUM
        lda     FNRPLY+SPLAYER+PLSCORE+1,x
SCNUM:  sta     MNUMH
        lda     #5
        jsr     MDEC
        jmp     FNENDW

; the title, the colony, the footer, a row a frame
SCTITLE: lda    #0
        jsr     FNROWA
        lda     HPHASE
        cmp     #PHOVER
        beq     SCR1
        ldx     #(SSUMM)&$FF
        ldy     #(SSUMM)>>8
        jsr     FNSTRXY
        lda     HROUND
        ldx     #2
        jsr     MDEC8
        jmp     SCR2
SCR1:   ldx     #(SOVER)&$FF
        ldy     #(SOVER)>>8
        jsr     FNSTRXY
SCR2:   jmp     FNENDW
SCCOL:  lda     #17
        jsr     FNROWA
        ldx     #(SCOLS)&$FF
        ldy     #(SCOLS)>>8
        jsr     FNSTRXY
        jsr     DEC24
        jmp     FNENDW
; my flag
SCLAST:
        lda     #0
        sta     SRDY
        ldx     HSEAT
        cpx     #4
        bcs     SCR3
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLFLAGS,x
        and     #PF_READY
        sta     SRDY
SCR3:   lda     HPHASE
        cmp     #PHOVER
        beq     SCOVER
        jsr     FOOTER
        jmp     SCDONE
; game over: the first founder, and the rating
SCOVER: lda     #19
        jsr     FNROWA
        ldx     #0
SCO1:   lda     PLOFS,x
        tay
        lda     FNRPLY+SPLAYER+PLRANK,y
        cmp     #1
        beq     SCO2
        inx
        cpx     #4
        bne     SCO1
        ldx     #0
SCO2:   lda     PLOFS,x
        tax
        ldy     #8
SCO3:   lda     FNRPLY+SPLAYER+PLNAME,x
        beq     SCO4
        jsr     FNCHR
        inx
        dey
        bne     SCO3
SCO4:   ldx     #(SFOUND)&$FF
        ldy     #(SFOUND)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     #20
        jsr     FNROWA
        ldx     SRATE
        cpx     #7
        bcc     SCO5
        ldx     #6
SCO5:   lda     RATELO,x
        ldy     RATEHI,x
        tax
        jsr     FNSTRXY
        jsr     FNENDW
SCDONE: lda     #0
        sta     MCOMP
        lda     HFLAGS
        and     #~HFNEW&$FF
        sta     HFLAGS
        rts

; FOOTER -- what FIRE does now.
FOOTER: lda     #19
        jsr     FNROWA
        ldx     #(SWAITD)&$FF
        ldy     #(SWAITD)>>8
        lda     HSEAT
        cmp     #4
        bcs     FT1
        lda     SRDY
        ora     SPRESS
        bne     FT1
        jsr     SDRAWN
        bcc     FT1
        ldx     #(SPRESSS)&$FF
        ldy     #(SPRESSS)>>8
FT1:    jsr     FNSTRXY
        jmp     FNENDW

; SDRAWN -- carry set once the summary has been up DRAWN jiffies (the
; elapsed at the last arrival, plus the frames since).
SDRAWN: lda     MFRAME
        sec
        sbc     HRXF
        sta     MTMP
        lda     MFRAMEH
        sbc     HRXFH
        sta     MTMP2
        lda     MTMP
        clc
        adc     SEL0
        sta     MTMP
        lda     MTMP2
        adc     SEL1
        cmp     #(DRAWN)>>8
        bne     SDR1
        lda     MTMP
        cmp     #(DRAWN)&$FF
SDR1:   rts

; DEC24 -- STOT0-2 as up to 8 digits, no leading zeros. Repeated
; subtraction by powers of ten, 24 bits (a colony total under 16.7 million).
DEC24:  ldx     #0
        stx     MDGT            ; bit 7: a digit has printed
D241:   ldy     #0
D242:   lda     STOT0
        sec
        sbc     P10A,x
        sta     MTMP
        lda     STOT1
        sbc     P10B,x
        sta     MTMP2
        lda     STOT2
        sbc     P10C,x
        bcc     D243
        sta     STOT2
        lda     MTMP2
        sta     STOT1
        lda     MTMP
        sta     STOT0
        iny
        bne     D242
D243:   tya
        bne     D244
        bit     MDGT
        bmi     D244
        cpx     #7
        bne     D245
D244:   lda     #$80
        sta     MDGT
        tya
        ora     #'0'
        jsr     FNCHR
D245:   inx
        cpx     #8
        bne     D241
        rts
P10A:   DB      (10000000)&$FF,(1000000)&$FF,(100000)&$FF,(10000)&$FF,(1000)&$FF,(100)&$FF,(10)&$FF,1
P10B:   DB      ((10000000)>>8)&$FF,((1000000)>>8)&$FF,((100000)>>8)&$FF,((10000)>>8)&$FF,((1000)>>8)&$FF,0,0,0
P10C:   DB      (10000000)>>16,(1000000)>>16,(100000)>>16,0,0,0,0,0

; ---------------------------------------------------------------------------
APPVBL: jsr     SNDRUN
        jsr     SCOMPOSE
        jsr     TICKER
        jsr     INSCAN
        jsr     RSTCHK
        lda     HPHASE
        cmp     #PHSUMM
        bne     AP9
        lda     MCOMP
        bne     AP9
        lda     HSEAT
        cmp     #4
        bcs     AP9
        lda     SPRESS
        ora     SRDY
        bne     AP9
        jsr     SDRAWN
        bcc     AP9
        lda     SBELL           ; the summary is drawn: ring, once
        bne     AP5
        inc     SBELL
        lda     #SNDBELL
        jsr     SNDPLAY
        jsr     FOOTER
        lda     SMSGK           ; "press all player buttons": at the next
        bne     AP5             ;   state (the reply window is whole then)
        lda     #1
        sta     SMSGK
AP5:    lda     MINP
        and     #IN_FIRE
        beq     AP9
        inc     SPRESS
        lda     #RQREADY
        ldx     #0
        ldy     #0
        jsr     QACT
        jmp     FOOTER
AP9:    rts

STICK:  rts

PLOFS:  DB      0,PLSIZE,2*PLSIZE,3*PLSIZE
SEATROW: DB     1,5,9,13
MULEV:  DB      25+35,50+35,75+35,100+35

SSUMM:  DB      "SUMMARY #",0
SOVER:  DB      " GAME OVER",0
SMONEY: DB      "MONEY ",0
SLANDS: DB      "LAND  ",0
STOTAL: DB      "TOTAL ",0
SCOLS:  DB      "COLONY ",0
SWAITD: DB      "  WAITING",0
SPRESSS: DB     "FIRE: READY",0
SFOUND: DB      " FOUNDER",0
RT0:    DB      "COLONY DIED",0
RT1:    DB      "IT FAILED",0
RT2:    DB      "BARELY LIVED",0
RT3:    DB      "A SUCCESS",0
RT4:    DB      "GREAT",0
RT5:    DB      "TREMENDOUS",0
RT6:    DB      "PHENOMENAL!",0

MBGT:   DB      CBLACK
        DB      CPLAY0,CPLAY0,CPLAY0,CPLAY0
        DB      CPLAY1,CPLAY1,CPLAY1,CPLAY1
        DB      CPLAY2,CPLAY2,CPLAY2,CPLAY2
        DB      CPLAY3,CPLAY3,CPLAY3,CPLAY3
        DB      CBLACK,CBLACK,CBLACK,CBLACK,CBLACK

        INCLUDE "mlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "scrloop.inc"
        INCLUDE "ticker.inc"
        INCLUDE "disptext.inc"

; ---------------- data: tables, not code (tools/dataranges.py) ----------
DSTART:
RATELO: DB      (RT0)&$FF,(RT1)&$FF,(RT2)&$FF,(RT3)&$FF,(RT4)&$FF,(RT5)&$FF,(RT6)&$FF
RATEHI: DB      (RT0)>>8,(RT1)>>8,(RT2)>>8,(RT3)>>8,(RT4)>>8,(RT5)>>8,(RT6)>>8

        END
