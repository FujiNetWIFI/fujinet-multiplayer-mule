; tables.asm -- bank 8: the table list.
;
; /tables?bin=1 is a count and then 33-byte records (id[8], name[20],
; players[5] "n / 4"). The stick picks, FIRE joins -- the id goes into path
; buffer 1 and appkey 10, and the first /state takes a seat (the lobby bank
; takes it from there). RIGHT asks again.

        CPU     6502
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     0
MHASCMP EQU     0

        INCLUDE "../build/tail.inc"

LSEL    EQU     PV+0            ; the list cursor
LCNT    EQU     PV+1            ; tables in the list
RL0     EQU     3               ; the list's first row

        ORG     $1000

TENTRY: lda     MENT
        cmp     #ENRUN
        bne     TE1
        jmp     SLOOP
TE1:    cmp     #ENTABLE
        bne     LLIST
        jmp     LTABLE

; ---------------------------------------------------------------------------
; ENLIST: ask for the list.
; With a table already joined (the room appkey named one), the first /state
; is the join instead.
LLIST:  lda     #BKTABLE
        sta     MSCR
        lda     HFLAGS          ; just left a table: the room forgets it --
        and     #HFLEFT         ;   appkey 10 = "<endpoint>/", no table
        beq     LL0
        eor     HFLAGS
        sta     HFLAGS
        lda     #2
        sta     VBLANK
        lda     #PBEND
        jsr     FNWSEL
        lda     FNPLNL
        clc
        adc     #1
        ldx     #AKROOM
        jsr     AKWBEG
        bne     LL0
        lda     #PBEND
        jsr     FNWSEL
        jsr     FNWRAW
        lda     #'/'
        sta     FNTX
        jsr     FNGO
LL0:    jsr     LSCREEN
        lda     #0
        sta     LCNT
        sta     MBGMSK
        lda     #$FF
        sta     MBAR
        lda     #RL0
        jsr     FNROWA
        lda     HFLAGS
        and     #HFTBL
        beq     LL1
        ldx     #(SJOINS)&$FF
        ldy     #(SJOINS)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     #0
        sta     SNOPOLL
        lda     #1
        sta     NPOLL
        jmp     SLOOPF
LL1:    ldx     #(SLOAD)&$FF
        ldy     #(SLOAD)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     #1
        sta     SNOPOLL
        lda     #RQTABLE
        jsr     QACT
        jmp     SLOOPF

; LSCREEN -- a fresh text screen: the kernel's TIA setup, every row blank,
; the title. Between frames, in the overscan.
LSCREEN: jsr    DINIT
        jsr     FNCLS
        lda     #0
        jsr     FNROWA
        ldx     #(STITLE)&$FF
        ldy     #(STITLE)>>8
        jsr     FNSTRXY
        jmp     FNENDW

; ---------------------------------------------------------------------------
; ENTABLE: the list is in the window (or HFERR says it is not).
LTABLE: lda     #0
        sta     MBGMSK
        sta     LSEL
        sta     LCNT
        lda     HFLAGS
        and     #HFERR
        bne     LT1
        lda     FNRPLY
        cmp     #MAXTBL+1
        bcc     LT0
        lda     #MAXTBL
LT0:    sta     LCNT
LT1:    jsr     LSCREEN
        lda     #1
        jsr     FNROWA
        ldx     #(STABLS)&$FF
        ldy     #(STABLS)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     LCNT
        bne     LT2
        lda     #RL0
        jsr     FNROWA
        ldx     #(SNONE)&$FF
        ldy     #(SNONE)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     #$FF
        sta     MBAR
        jmp     LT4
LT2:    lda     #0
        sta     MIDX
LT3:    lda     MIDX
        clc
        adc     #RL0
        jsr     FNROWA
        jsr     LRECP
        ldy     #TBNAME
        jsr     LRFLD
        jsr     FNENDW
        inc     MIDX
        lda     MIDX
        cmp     LCNT
        bne     LT3
        jsr     LSEAT
LT4:    lda     #18
        jsr     FNROWA
        ldx     #(SJOIN)&$FF
        ldy     #(SJOIN)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     #19
        jsr     FNROWA
        ldx     #(SRELD)&$FF
        ldy     #(SRELD)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        lda     #1
        sta     SNOPOLL
        jmp     SLOOPF

; LRECP -- FNPTRL/H at table record MIDX (records from window offset 1).
LRECP:  lda     #(FNRPLY+1)&$FF
        sta     FNPTRL
        lda     #(FNRPLY+1)>>8
        sta     FNPTRH
        ldx     MIDX
        beq     LRP2
LRP1:   lda     FNPTRL
        clc
        adc     #TBSIZE
        sta     FNPTRL
        bcc     LRP3
        inc     FNPTRH
LRP3:   dex
        bne     LRP1
LRP2:   rts

; LRFLD -- append the record's field at Y, up to 12 characters, to its NUL.
LRFLD:  ldx     #FNTCOL
LRF1:   lda     (FNPTRL),y
        beq     LRF2
        sta     FNRSEL+FH_TCHR
        iny
        dex
        bne     LRF1
LRF2:   rts

; LSEAT -- the cursor: the bar, and the chosen table's seats.
LSEAT:  lda     LSEL
        clc
        adc     #RL0
        sta     MBAR
        lda     #13
        jsr     FNROWA
        ldx     #(SSEATS)&$FF
        ldy     #(SSEATS)>>8
        jsr     FNSTRXY
        lda     LSEL
        sta     MIDX
        jsr     LRECP
        ldy     #TBPLYRS
        jsr     LRFLD
        jmp     FNENDW

; LJOIN -- the chosen table's id into path buffer 1 and appkey 10, then the
; first /state, which takes the seat.
LJOIN:  lda     LSEL
        sta     MIDX
        jsr     LRECP
        lda     #PBTABLE
        jsr     FNWSEL
        jsr     FNWRST
        ldy     #TBID
        ldx     #0
LJ1:    lda     (FNPTRL),y
        beq     LJ2
        cmp     #' '
        beq     LJ2
        sta     FNRSEL+FH_PATHC
        iny
        inx
        cpx     #8
        bne     LJ1
LJ2:
; appkey 10 = <endpoint>/?table=<id>: best effort. The endpoint's length
; is the cart's to say (FNPLNL, for the selected buffer).
        stx     MTMP            ; the id's length
        lda     #2
        sta     VBLANK
        lda     #PBEND
        jsr     FNWSEL
        lda     FNPLNL
        clc
        adc     #8
        adc     MTMP
        ldx     #AKROOM
        jsr     AKWBEG
        bne     LJ3
        lda     #PBEND
        jsr     FNWSEL
        jsr     FNWRAW
        ldx     #(SROOM)&$FF
        ldy     #(SROOM)>>8
        jsr     LTXS
        lda     #PBTABLE
        jsr     FNWSEL
        jsr     FNWRAW
        jsr     FNGO
LJ3:    lda     HFLAGS
        ora     #HFTBL
        sta     HFLAGS
        lda     #$FF
        sta     HSCRB
        lda     #0
        sta     SNOPOLL
        lda     #1
        sta     NPOLL
        lda     #RL0
        jsr     FNROWA
        ldx     #(SJOINS)&$FF
        ldy     #(SJOINS)>>8
        jsr     FNSTRXY
        jsr     FNENDW
        jmp     SLOOPF

; LTXS -- append the NUL-terminated string at X/Y to TX.
LTXS:   stx     FNPTRL
        sty     FNPTRH
        ldy     #0
LTX1:   lda     (FNPTRL),y
        beq     LTX2
        sta     FNTX
        iny
        bne     LTX1
LTX2:   rts

; ---------------------------------------------------------------------------
; APPVBL: input, sound, a step of the redraw.
APPVBL: jsr     SNDRUN
        jsr     INREPT
        lda     MINP
        and     #IN_UP
        beq     LA1
        lda     LSEL
        beq     LA1
        dec     LSEL
        jsr     LSEAT
LA1:    lda     MINP
        and     #IN_DOWN
        beq     LA2
        lda     LSEL
        clc
        adc     #1
        cmp     LCNT
        bcs     LA2
        sta     LSEL
        jsr     LSEAT
LA2:    lda     MINP
        and     #IN_RIGHT
        beq     LA3
        lda     NSTATE
        bne     LA3
        lda     #RQTABLE
        jsr     QACT
LA3:    lda     MINP
        and     #IN_FIRE
        beq     LA4
        lda     LCNT
        beq     LA3A
        lda     NSTATE          ; not while a request is in flight
        bne     LA4
        jmp     LJOIN
LA3A:   lda     #RQTABLE        ; nothing listed: FIRE asks again
        jsr     QACT
LA4:    rts


STICK:  rts

STITLE: DB      "  M.U.L.E.",0
STABLS: DB      "   TABLES",0
SLOAD:  DB      "  LOADING",0
SNONE:  DB      " NO TABLES",0
SSEATS: DB      "SEATS ",0
SJOIN:  DB      "FIRE: JOIN",0
SRELD:  DB      "RIGHT:RELOAD",0
SJOINS: DB      "  JOINING",0
SROOM:  DB      "/?table=",0

MBGT:   DB      0

        INCLUDE "mlib.inc"
        INCLUDE "appkey.inc"
        INCLUDE "sound.inc"
        INCLUDE "scrloop.inc"
        INCLUDE "disptext.inc"

        END
