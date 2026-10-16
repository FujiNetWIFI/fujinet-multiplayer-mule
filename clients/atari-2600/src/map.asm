; map.asm -- bank 4: the map, watched. The land grant, production, and
; everyone else's turn in development.
;
; The map itself is the cartridge's: FN_BLIT_MULEMAP composes the six
; playfield tables straight out of the state in the reply window. What this
; bank draws around it is the text: the phase and round at the top, a line
; under the map.
;
; THE LAND GRANT AND PRODUCTION think in bank 12 (mapx.inc), once a frame
; from STICK: the grant's cursor and claim, round 1's transport ship, the
; colony event's plot. The box is player 0, double width, the size of a plot;
; the ship is player 1.

        CPU     6502UNDOC
        INCLUDE "vcs.inc"
        INCLUDE "fujinet.inc"
        INCLUDE "mdefs.inc"
MHASINP EQU     1
MHASDEC EQU     1
MHASCMP EQU     0
MHASMSG EQU     1
TICKROW EQU     1

        INCLUDE "../build/tail.inc"

        INCLUDE "mapram.inc"

        ORG     $1000

MENTRY: lda     MENT
        cmp     #ENRUN
        bne     ME0
        jmp     SLOOP
ME0:    cmp     #ENPOST
        bne     ME1
        jmp     SLOOP1
ME1:    cmp     #ENSTATE
        bne     MSETUP
        jmp     MSTATE

; ---------------------------------------------------------------------------
; ENENTER: a new screen, between frames.
MSETUP: lda     #BKMAP
        sta     MSCR
        lda     #2
        sta     VBLANK
        jsr     DINIT
        jsr     FNCLS
        lda     #0
        sta     SNOPOLL
        sta     GCLAIM
        sta     FIGNUS
        sta     BARPF1
        sta     BARPF2
        sta     BARPF1R
        sta     BARPF2R
        lda     #CWHITE
        sta     TXCOL
        lda     #$FF
        sta     GCUR
        sta     MLAST
        sta     GPH
        lda     #0
        sta     MSGCNT
        lda     #16             ; the cursor's height
        sta     P0HV
        lda     #200            ; both players off the map
        sta     P0TOP
        sta     P1TOP
        jsr     SETPTR
        jsr     MREAD
        jmp     SLOOPF
MSTATE: jsr     MREAD           ; inside a frame's overscan: no fresh start
        jmp     SLOOP

; MREAD -- a state has arrived: the map, the text, the grant's cursor.
MREAD:  lda     #0
        ldx     HPHASE
        cpx     #PHPROD
        bne     MR1
        lda     #MMPROD
MR1:    tax
        lda     #(SMAP)&$FF
        ldy     #0
        jsr     FNBARG
        lda     #(SMAP)>>8
        sta     FNRSEL+FH_BSH
        lda     #FB_MULEMAP
        jsr     FNBLIT
        lda     #1              ; the text, a row a frame (MCOMPOSE); bank 12
        sta     MCOMP           ;   reads the grant's and production's parts
        sta     GFRESH          ;   on its next visit
        jmp     MMSG

; MCOMPOSE -- the text rows, one a frame from the vblank; HFNEW is cleared
; with the last.
MCOMPOSE: lda   MCOMP
        bne     MC0
        rts
MC0:    inc     MCOMP
        cmp     #1
        bne     MC2
; the top row: the phase and the round
        lda     #0
        jsr     FNROWA
        ldx     HPHASE
        lda     PHNLO,x
        ldy     PHNHI,x
        tax
        jsr     FNSTRXY
        lda     #' '
        jsr     FNCHR
        lda     HROUND
        ldx     #2
        jsr     MDEC8
        jsr     FNENDW
        rts
; the bottom row: my money, or SPECTATOR (no seat)
MC2:
        lda     #MBOTROW
        jsr     FNROWA
        lda     HSEAT
        cmp     #4
        bcc     MC2A
        ldx     #(SSPECT)&$FF
        ldy     #(SSPECT)>>8
        jsr     FNSTRXY
        jmp     MC2B
MC2A:   lda     #'$'
        jsr     FNCHR
        lda     HMONEY
        sta     MNUM
        lda     HMONEYH
        sta     MNUMH
        lda     #5
        jsr     MDEC
MC2B:   jsr     FNENDW
        lda     #0
        sta     MCOMP
        lda     HFLAGS
        and     #~HFNEW&$FF
        sta     HFLAGS
        rts


; MMSG -- what the map says, once each (MLAST): the land for sale and its
; opening price; the colony's event at production; whose turn it is.
MMSG:   lda     HPHASE
        cmp     #PHLAUC
        bne     MM2
        lda     FNRPLY+SAPLOT   ; the plot for sale: boxed (it blinks)
        sta     GCUR
        jsr     GPLACE
        lda     FNRPLY+SAPLOT
        ora     #$C0
        cmp     MLAST
        beq     MM9
        sta     MLAST
        lda     FNRPLY+SASTART
        sta     MNUM
        lda     FNRPLY+SASTART+1
        sta     MNUMH
        lda     #MSBIDS
        bne     MM8
MM2:    cmp     #PHPROD
        bne     MM4
        lda     #1              ; the colony block is in the second slice
        jsr     SLICE
        lda     FNRPLY+SCEVENT-512
        pha
        lda     #0
        jsr     SLICE
        pla
        beq     MM9
        cmp     #10
        bcs     MM9
        ora     #$40
        cmp     MLAST
        beq     MM9
        sta     MLAST
        and     #$0F
        clc
        adc     #MSCOLONY
        bne     MM8
MM4:    cmp     #PHDEV
        bne     MM9
        ldx     FNRPLY+STSEAT   ; someone's turn: said once a seat
        cpx     #4
        bcs     MM9
        txa
        ora     #$20
        cmp     MLAST
        beq     MM9
        sta     MLAST
        lda     PLOFS,x
        tax
        lda     FNRPLY+SPLAYER+PLFLAGS,x
        and     #PF_AI
        beq     MM5
        lda     #MSCPU
        bne     MM8
MM5:    lda     #MSTURN
MM8:    sta     MSGREQ
MM9:    rts

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

; GPLACE -- the cursor box over plot GCUR (or off the map).
GPLACE: lda     GCUR
        cmp     #NPLOTS
        bcc     GP1
        lda     #200
        sta     P0TOP
        jmp     SETPTR
GP1:    ldx     #0
GP2:    cmp     #9
        bcc     GP3
        sbc     #9
        inx
        bne     GP2
GP3:    tay                     ; column
        lda     PLOTX,y
        sta     FIGX
        lda     ROWENT,x
        sta     P0TOP
        lda     #5              ; double width: a plot's sixteen clocks
        sta     FIGNUS
        lda     #CWHITE
        sta     FIGCOL
        lda     #(CURSOR)&$FF
        sta     MTMP
        lda     #(CURSOR)>>8
        sta     MTMP2
; SETPTR -- the shape pointers: shape - top entry, so (ptr),y with y the
; entry lands on the shape's first row at the top entry.
SETPTR: lda     #(CURSOR)&$FF
        sec
        sbc     P0TOP
        sta     P0PTR
        lda     #(CURSOR)>>8
        sbc     #0
        sta     P0PTRH
        lda     #(CURSOR)&$FF
        sec
        sbc     P1TOP
        sta     P1PTR
        lda     #(CURSOR)>>8
        sbc     #0
        sta     P1PTRH
        rts

; ---------------------------------------------------------------------------
; APPVBL: sound, input, the cursor's local steps.
APPVBL: jsr     SNDRUN
        jsr     MCOMPOSE
        jsr     TICKER
; the land for sale blinks: 16 frames on, 16 off
        lda     HPHASE
        cmp     #PHLAUC
        bne     APB
        lda     MFRAME
        and     #16
        beq     APB1
        lda     #200
        sta     P0TOP
        bne     APB2
APB1:   jsr     GPLACE
APB2:   jsr     INSCAN          ; the button skips the wait
        and     #IN_FIRE
        beq     APB3
        ldx     HSEAT           ; (not a spectator's)
        cpx     #4
        bcs     APB3
        lda     #RQREADY
        ldx     #0
        ldy     #0
        jsr     QACT
APB3:   jmp     RSTCHK
APB:    jsr     INSCAN
        jmp     RSTCHK

; STICK -- the grant and production: bank 12, once a frame (it comes back
; to SLOOP1 with ENPOST).
STICK:  lda     HPHASE
        cmp     #PHGRANT
        beq     STK1
        cmp     #PHPROD
        bne     STK9
STK1:   lda     #ENRUN
        sta     MENT
        lda     #BKTOWNX
        jmp     MGOTO
STK9:   rts

; plot column -> POSOBJ x of its left edge (double-width player: POSOBJ's
; x is 4 clocks right of the picture, measured), row -> top entry
PLOTX:  DB      12,28,44,60,76,92,108,124,140
PLOFS:  DB      0,PLSIZE,2*PLSIZE,3*PLSIZE
ROWENT: DB      0,16,32,48,64

PHN0:   DB      "LOBBY",0
PHN1:   DB      "LAND GRANT",0
PHN2:   DB      "LAND SALE",0
PHN3:   DB      "DEVELOP",0
PHN4:   DB      "PRODUCTION",0
PHN5:   DB      "AUCTION",0
PHN6:   DB      "AUCTION",0
PHN7:   DB      "STATUS",0
PHN8:   DB      "GAME OVER",0
SSPECT: DB      " SPECTATOR",0

MBGT:   DB      0

MT0     EQU     FNTEXT+$000+MAP0
MT1     EQU     FNTEXT+$080+MAP0
MT2     EQU     FNTEXT+$100+MAP0
MT3     EQU     FNTEXT+$180+MAP0
MT4     EQU     FNTEXT+$200+MAP0
MT5     EQU     FNTEXT+$280+MAP0

        INCLUDE "mlib.inc"
        INCLUDE "sound.inc"
        INCLUDE "scrloop.inc"
        INCLUDE "ticker.inc"
        INCLUDE "dispmap.inc"

; ---------------- data: tables, not code (tools/dataranges.py) ----------
DSTART:
PHNLO:  DB      (PHN0)&$FF,(PHN1)&$FF,(PHN2)&$FF,(PHN3)&$FF,(PHN4)&$FF,(PHN5)&$FF,(PHN6)&$FF,(PHN7)&$FF,(PHN8)&$FF
PHNHI:  DB      (PHN0)>>8,(PHN1)>>8,(PHN2)>>8,(PHN3)>>8,(PHN4)>>8,(PHN5)>>8,(PHN6)>>8,(PHN7)>>8,(PHN8)>>8
; The kernel's colour per entry: the map model's (tools/mulemap.py COLTAB).
; It may not cross a page (a kernel load would take a fifth cycle), so it
; moves to the next page only if it must.
        IF      (* & $FF) > $100 - 80
        ALIGN   256
        ENDIF
        INCLUDE "../build/mapcol.inc"
; At fixed addresses (mapram.inc), for bank 12; neither crosses a page.
        IF      * > MSHIP
        ERROR   "map.asm: the bank runs into MSHIP"
        ENDIF
        ORG     MSHIP
; The transport ship, nose left, 8 entries.
SHIPPIC: DB     $0C,$1E,$7F,$FF,$7F,$1E,$0C,$00
; The cursor: a box a plot big (double width), 16 entries.
CURSOR: DB      $FF,$81,$81,$81,$81,$81,$81,$81
        DB      $81,$81,$81,$81,$81,$81,$81,$FF
        IF      CURSOR <> MCURS
        ERROR   "map.asm: CURSOR is not at MCURS"
        ENDIF

        END
