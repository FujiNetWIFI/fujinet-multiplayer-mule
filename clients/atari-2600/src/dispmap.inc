; dispmap.inc -- the map screen: two text rows, the 9 x 5 map, a bar, a
; text row.
;
; The map is the playfield, rewritten six times a scanline from six tables
; the cartridge composes (tools/mulemap.py is the model; fn-2600's
; FN_BLIT_MULEMAP makes the bytes): 80 ENTRIES of two scanlines, 16 to a plot
; row, each drawn in the colour MCOLTAB gives it. Player 0 is the walking
; figure (or, double width, the cursor), player 1 the M.U.L.E.; each is
; drawn at entry resolution by a skipdraw, P0 on the first line of an entry
; and P1 on the second.
;
; The including bank supplies:
;   MT0..MT5   the six tables' entry 0 (planes, or ROM: the layout build,
;              the town)
;   MCOLTAB    the colour of each entry
;   MAPENT     entries (80 unless given), P1H player 1's height (8)
;   MBGT       the text kernel's row colours are not used here
;   APPVBL     the per-frame hook
;
; Visible band (192, the last lines a timed pad):
;    1  unblank
;   12  text rows 0 and 1          (players positioned for text in vblank)
;    3  position the figure and the M.U.L.E., HMOVE
;  160  the map
;    2  the time bar (a playfield bar), which is also the lead-out
;    2  position the players for text again
;    6  text row MBOTROW
;    6  pad (timed)

; ---------------- kernel timing constants (disptext.inc's) ----------------
RNOP    EQU     16
R3      EQU     1
HM0     EQU     15
HM1     EQU     0

TP0     EQU     FNTEXT+$000
TP1     EQU     FNTEXT+$080
TP2     EQU     FNTEXT+$100
TP3     EQU     FNTEXT+$180
TP4     EQU     FNTEXT+$200
TP5     EQU     FNTEXT+$280
TINK    EQU     FNTCLH-1

VBTIM   EQU     43
OSTIM   EQU     35
VITIM   EQU     227

        IFNDEF  MAPENT
MAPENT  EQU     80              ; entries, 2 scanlines each (the town: 72)
        ENDIF
MAP0    EQU     12              ; the plane byte of entry 0
MBOTROW EQU     17              ; the text row under the map

CGROUND EQU     $08             ; the Atari map's grey
        IFNDEF  MAPBG
MAPBG   EQU     CGROUND         ; the background (the town's is black)
        ENDIF

; The kernel's cells, first in the phase area (mdefs.inc PV).
FIGX    EQU     PV+0            ; player 0's x, colour clocks 0-159
MULX    EQU     PV+1            ; player 1's x
P0PTR   EQU     PV+2            ; shape - top entry, for (P0PTR),y
P0PTRH  EQU     PV+3
P1PTR   EQU     PV+4
P1PTRH  EQU     PV+5
P0TOP   EQU     PV+6            ; top entry of player 0 (0-79; >=80 hides)
P1TOP   EQU     PV+7
P0CNT   EQU     PV+8            ; the skipdraw counters, set per frame
P1CNT   EQU     PV+9
FIGCOL  EQU     PV+10           ; COLUP0 / COLUP1 over the map
MULCOL  EQU     PV+11
FIGNUS  EQU     PV+12           ; NUSIZ0 over the map: 0, or 5 = the cursor;
                                ;   bit 3 is also REFP0's (a left-facing figure)
BARPF1  EQU     PV+13           ; the time bar's playfield, left half
BARPF2  EQU     PV+14
BARPF1R EQU     PV+15           ;   and right half (PF0 unused)
BARPF2R EQU     PV+16
BARCOL  EQU     PV+17
TXCOL   EQU     PV+18           ; the text rows' ink
P0HV    EQU     PV+19           ; player 0's height in entries (8, or 16)
MULREF  EQU     PV+20           ; REFP1 over the map ($08: mirrored)
MKEND   EQU     PV+21           ; the phase's own cells start here

        IFNDEF  P1H
P1H     EQU     8               ; player 1's height, in entries (town: 12)
        ENDIF

; ---------------- DINIT ----------------
DINIT:  lda     #0
        sta     CTRLPF
        sta     PF0
        sta     PF1
        sta     PF2
        sta     ENAM0
        sta     ENAM1
        sta     ENABL
        sta     RESMP0
        sta     RESMP1
        sta     REFP0
        sta     REFP1
        sta     HMCLR
        sta     COLUBK
        sta     GRP0
        sta     GRP1
        sta     GRP0
        rts

; ---------------- the frame ----------------
DFRESH: lda     #OSTIM
        sta     TIM64T
DLOOP:  jsr     DFRAME
        jmp     DLOOP

DFRAME: bit     TIMINT
        bpl     DFRAME
        sta     WSYNC
        lda     #2
        sta     VSYNC
        sta     WSYNC
        sta     WSYNC
        sta     WSYNC
        lda     #0
        sta     VSYNC
        lda     #2
        sta     VBLANK
        lda     #VBTIM
        sta     TIM64T
        inc     MFRAME
        bne     DFR1
        inc     MFRAMEH
DFR1:   jsr     APPVBL

DFRAME2:
; The players for the text, in the vblank: disptext.inc's positioning.
        jsr     TXTPOS
; The skipdraw counters: the draw window is [TOP, TOP+H). P0HV holds the
; height and the kernel compares with height - 1: DCP's carry is A >= M.
        lda     P0TOP
        clc
        adc     P0HV
        sta     P0CNT
        dec     P0HV
        lda     P1TOP
        clc
        adc     #P1H
        sta     P1CNT
        lda     #0
        sta     COLUBK
        sta     PF0
        sta     PF1
        sta     PF2
        lda     TXCOL
        sta     COLUP0
        sta     COLUP1
DVBLP:  bit     TIMINT
        bpl     DVBLP
        sta     WSYNC
        lda     #0
        sta     VBLANK
        lda     #VITIM
        sta     TIM64T

; ---- two text rows ----
        ldy     #0
        jsr     TINK5
        jsr     TINK5

; ---- the figures ----
        lda     FIGX
        ldx     #0
        jsr     POSOBJ
        lda     MULX
        ldx     #1
        jsr     POSOBJ
        sta     WSYNC
        sta     HMOVE           ; 3: the comb is over clocks 0-7, left of
        lda     #MAPBG          ;   the map
        sta     COLUBK          ; 8: the ground, under the comb
        lda     FIGNUS
        sta     NUSIZ0
        sta     REFP0
        lda     MULREF
        sta     REFP1
        lda     #0
        sta     NUSIZ1
        sta     VDELP0
        sta     VDELP1
        lda     FIGCOL
        sta     COLUP0
        lda     MULCOL
        sta     COLUP1

; ---- the map: 80 entries of two lines ----
; LINE A: colour, the six playfield writes, player 0.
;   PF0L by 25 (the map starts at clock 8), PF1L by 28, PF2L by 38; the right
;   half PF0 in 28-49, PF1 in 39-54, PF2 in 50-65. COLUPF by 25.
; LINE B: the six writes again (the right half's values are still in the
; registers), player 1, the loop.
; GRP0 lands at 67 of line A and GRP1 at 39 of line B, so a figure left of
; those points shows each row one line late -- the same row on both lines,
; just lower -- and only a figure straddling clock 133 (P0) or 49 (P1)
; tears.
        ldy     #0
MKA:    sta     WSYNC           ; 0
        lda     MCOLTAB,y       ; 4
        sta     COLUPF          ; 7
        lda     MT0,y           ; 11
        sta     PF0             ; 14
        lda     MT1,y           ; 18
        sta     PF1             ; 21
        lda     MT2,y           ; 25
        sta     PF2             ; 28
        lda     MT3,y           ; 32
        sta     PF0             ; 35
        lda     MT4,y           ; 39
        sta     PF1             ; 42
        ldx     MT5,y           ; 46
        lda     P0HV            ; 49    height - 1: see DFRAME2
        dcp     P0CNT           ; 54
        stx     PF2             ; 57    (flags untouched)
        bcs     MKA1            ; 59/60
        lda     #0              ; 61
        DB      $2C             ; 65    bit abs: skips the lda (),y
MKA1:   lda     (P0PTR),y       ; 65/66
        sta     GRP0            ; 68/69
; LINE B
        sta     WSYNC           ; 0
        lda     MT0,y           ; 4
        sta     PF0             ; 7
        lda     MT1,y           ; 11
        sta     PF1             ; 14
        lda     MT2,y           ; 18
        sta     PF2             ; 21
        lda     #P1H-1          ; 23
        dcp     P1CNT           ; 28
        bcs     MKB1            ; 30/31
        lda     #0              ; 32
        DB      $2C             ; 36
MKB1:   lda     (P1PTR),y       ; 36/37
        sta     GRP1            ; 39/40
        lda     MT3,y           ; 43
        sta     PF0             ; 46
        lda     MT4,y           ; 50
        sta     PF1             ; 53    by 54
        lda     MT5,y           ; 57
        sta     PF2             ; 60
        iny                     ; 62
        cpy     #MAPENT         ; 64
        bne     MKA             ; 67

; ---- the time bar, which is also the lead-out ----
        sta     WSYNC
        inc     P0HV            ; 5: back to the height (see DFRAME2)
        lda     #0              ; 7
        sta     GRP0            ; 10
        sta     GRP1            ; 13
        sta     PF0             ; 16
        lda     BARCOL          ; 19
        sta     COLUPF          ; 22
        lda     BARPF1          ; 25
        sta     PF1             ; 28
        lda     BARPF2          ; 31
        sta     PF2             ; 34
        lda     BARPF1R         ; 37
        nop                     ; 39
        nop                     ; 41
        sta     PF1             ; 44
        lda     BARPF2R         ; 47
        nop                     ; 49
        nop                     ; 51
        sta     PF2             ; 54
        sta     WSYNC
        lda     BARPF1          ; the second line: the same bar
        sta     PF1
        lda     BARPF2
        sta     PF2
        lda     BARPF1R
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        sta     PF1
        lda     BARPF2R
        nop
        nop
        sta     PF2

; ---- the bottom text row ----
        jsr     TXTPOS          ; two lines; the playfield off in the first
        lda     TXCOL
        sta     COLUP0
        sta     COLUP1
        ldy     #MBOTROW*FNTCLH
        jsr     TINK5

; ---- the pad ----
DFPAD:  bit     TIMINT
        bpl     DFPAD
        sta     WSYNC
        lda     #2
        sta     VBLANK
        lda     #OSTIM
        sta     TIM64T
        rts

; ---------------------------------------------------------------------------
; TXTPOS -- two lines: the players where the 48-pixel text wants them
; (disptext.inc's DINIT), three close copies, VDEL; the playfield and the
; background off.
TXTPOS: sta     WSYNC
        lda     #0
        sta     REFP0
        sta     REFP1
        sta     COLUBK
        sta     PF0
        sta     PF1
        sta     PF2
        sta     GRP0
        sta     GRP1
        sta     GRP0
        sta     WSYNC
        REPT    RNOP
        nop
        ENDM
        IF      R3
        sta     PAD3
        ENDIF
        sta     RESP0
        sta     RESP1
        lda     #HM0*16
        sta     HMP0
        lda     #(HM1*16)&$FF
        sta     HMP1
        sta     WSYNC
        sta     HMOVE
        lda     #$03
        sta     NUSIZ0
        sta     NUSIZ1
        lda     #1
        sta     VDELP0
        sta     VDELP1
        rts

; POSOBJ -- one line: object X (0 = P0, 1 = P1) to colour clock A. The
; classic divide-by-15 loop; the HMOVE that finishes it is the caller's.
POSOBJ: sta     WSYNC           ; 0
        sec                     ; 2
POS1:   sbc     #15             ; 4
        bcs     POS1            ; 6/7 per 15 clocks
        eor     #7
        asl     a
        asl     a
        asl     a
        asl     a
        sta     HMP0,x
        sta     RESP0,x
        rts

; ---------------------------------------------------------------------------
; TINK5 -- a text row from plane byte Y: five ink lines and a seam line
; that blanks the players. Y is left at the next row. (dispgame.inc's.)
TINK5:  tsx
        stx     SAVSP
        tya
        clc
        adc     #TINK
        sta     MLINE
TI1:    sta     WSYNC
        lda     TP0,y           ; 4
        sta     GRP0            ; 7
        lda     TP1,y           ; 11
        sta     GRP1            ; 14
        lda     TP2,y           ; 18
        sta     GRP0            ; 21
        lda     TP4,y           ; 25
        tax                     ; 27
        txs                     ; 29
        lda     TP3,y           ; 33
        tax                     ; 35
        lda     TP5,y           ; 39
        stx     GRP1            ; 42
        tsx                     ; 44
        stx     GRP0            ; 47
        sta     GRP1            ; 50
        sty     GRP0            ; 53
        iny                     ; 55
        cpy     MLINE           ; 58
        bne     TI1             ; 61
        ldx     SAVSP
        txs
        sta     WSYNC
        lda     #0
        sta     GRP0
        sta     GRP1
        sta     GRP0
        iny
        rts
