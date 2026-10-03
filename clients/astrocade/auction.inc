; auction.inc -- the per-good status screen, declaring, and both auctions
; (auction.c). Page 4.
;
; The screen (the Atari's lines, squeezed; rows are ours):
;   rows 0-1            title and the good (land: the price line on row 2)
;   Atari 36..144       the arena: the store's sell line at 48, its buy line
;                       (and the land start price) at 118, 2 lines a step
;   row 9               BUY / SELL, or each player's price
;   row 10 money        row 11 units        row 12 the message
; The players stand in columns at colour clocks 39/63/87/111, feet on their
; price line.

PVP     DEFL    PHRAM
        PV      AUKIND,1
        PV      AUGOOD,1
        PV      AUSTAGE,1
        PV      AUNEW,1
        PV      AUSTEPV,1
        PV      AUROLE,4
        PV      AUPOS,8
        PV      AUTGT,8
        PV      AUBASE,2
        PV      AUON,1
        PV      AUTCK,1
        PV      AUPUSH,1        ; our stick as sent: 0 released, 1 up, 2 down
        PV      AUHELD,1        ; FRAMES (low byte) our goods line was let go
        PV      AULN1,1         ; the sellers' / buyers' lines (255 none)
        PV      AULN2,1
        PV      AULBL,8         ; row 9 per player: 0 label, 1 none, a price
        PV      AUHB,2          ; land: the price line shown
        PV      AUSB,2          ; land: "bids start at" shown
        PV      AUMSG,1
        PV      AUBLINK,1       ; seats whose sprite blinks (bit per seat)
        PV      AUSHOWT,1       ; per-good status: the step shown
        PV      AUBAR,4
        PV      AUBART,4
        PV      AUSTK,1         ; the store's stock of the good
        PV      AUI,1           ; scratch
        PV      AUJ,1
        PV      AUK,1
        PV      AUW,2
        PV      AULO,2
        PV      AUBUF,24
        IF      PVP > RAMEND
RAMEND  DEFL    PVP
        ENDIF

; the auction section (mule_proto.h MuleAuction), in the window at W_AUC
; and in HOT at HOTU
AO_KIND EQU     0
AO_GOOD EQU     1
AO_PLOT EQU     2
AO_FLAGS EQU    4
AO_WINNER EQU   5
AO_STAGE EQU    7
AO_STEP EQU     8
AO_TRADED EQU   9
AO_START EQU    10
AO_LO   EQU     14
AO_HI   EQU     16
AO_SBUY EQU     18
AO_SSELL EQU    20
AO_PRICE EQU    22
AO_TICKS EQU    28
AO_ROLE EQU     30
AO_POS  EQU     34
AO_TGT  EQU     42
AO_PUSH EQU     54

ST_SHOW EQU     0
ST_DECL EQU     1
ST_LEAD EQU     2
ST_ACTIVE EQU   3
ST_DONE EQU     4
RL_NONE EQU     0
RL_BUYER EQU    1
RL_SELLER EQU   2
AF_COLL EQU     4
AF_CRIT EQU     32

AUYBASE EQU     118
AUYSELL EQU     48
AUYBOT  EQU     144
AUSTUP  EQU     41              ; (118 - 36) / 2
AUSTDN  EQU     13              ; (144 - 118) / 2
AULINX  EQU     4               ; the dotted lines: bytes 4..33 (x 16..135)
AULINW  EQU     30
ROWLBL  EQU     9
ROWMON  EQU     10
ROWUNIT EQU     11
ROWMSG  EQU     12
BARBOT  EQU     112
BARMAX  EQU     80

COLX:   DB      39,63,87,111

; ---- the phase entries -----------------------------------------------------------
A4ENTER: LD     A,(PHS)
        CP      PH_LAUC
        JP      Z,LAENTER
        JP      GDENTER
A4ANIM: LD      A,(ANIM)
        CP      AN_AUC
        RET     NZ
        JP      AUISR

; ---- reading the state: the window while it arrives (MAPSRC), else HOT -------
; AGB: A = auction byte E. Preserves BC, DE, HL.
AGB:    PUSH    HL
        PUSH    DE
        LD      D,0
        LD      A,(MAPSRC)
        OR      A
        JR      Z,AGB1
        LD      HL,W_AUC
        ADD     HL,DE
        CALL    RXGETB
        JR      AGB2
AGB1:   LD      HL,HOTU
        ADD     HL,DE
        LD      A,(HL)
AGB2:   POP     DE
        POP     HL
        RET
; AGW: HL = auction word E. Preserves BC, DE.
AGW:    PUSH    DE
        PUSH    BC
        CALL    AGB
        LD      C,A
        INC     E
        CALL    AGB
        LD      H,A
        LD      L,C
        POP     BC
        POP     DE
        RET
; AGIW: HL = auction word E + 2 * seat A (pos, target). Preserves BC.
AGIW:   PUSH    DE
        ADD     A,A
        ADD     A,E
        LD      E,A
        CALL    AGW
        POP     DE
        RET

; PLB: A = seat A's player byte at HOT offset E (HP_*). Preserves BC, DE.
PLB:    PUSH    HL
        PUSH    DE
        LD      D,A
        LD      A,(MAPSRC)
        OR      A
        LD      A,D
        JR      Z,PLB1
        PUSH    AF
        LD      A,E
        ADD     A,9             ; HP_* + 9 = PL_*
        LD      E,A
        POP     AF
        CALL    WPLAYER
        CALL    RXGETB
        JR      PLB2
PLB1:   CALL    HPLAYER
        LD      A,E
        ADD     A,L
        LD      L,A
        JR      NC,PLB1A
        INC     H
PLB1A:  LD      A,(HL)
PLB2:   POP     DE
        POP     HL
        RET
; PLW: HL = seat A's player word at HOT offset E. Preserves BC, DE.
PLW:    PUSH    BC
        LD      B,A
        CALL    PLB
        LD      C,A
        LD      A,B
        INC     E
        CALL    PLB
        DEC     E
        LD      H,A
        LD      L,C
        POP     BC
        RET
; PLC: A = seat A's colour.
PLC:    PUSH    HL
        PUSH    DE
        LD      E,HP_COL
        CALL    PLB
        AND     3
        LD      E,A
        LD      D,0
        LD      HL,PLCOLT
        ADD     HL,DE
        LD      A,(HL)
        POP     DE
        POP     HL
        RET

; AUPOSP: HL = &AUPOS[A]; AUTGTP: &AUTGT[A].
AUPOSP: LD      HL,AUPOS
        JR      AUP1
AUTGTP: LD      HL,AUTGT
AUP1:   ADD     A,A
        ADD     A,L
        LD      L,A
        RET     NC
        INC     H
        RET
; AUKEEP: our released goods line (A_HOLD). The server's is behind what we
; showed and carries on to where we let go, so ours stays put rather than
; jump back, until the server's comes to rest (or a second passes).
; BC, DE = our pos and target before the snap.
AUKEEP: LD      A,(AUKIND)
        CP      2
        RET     NZ
        LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        RET     NZ
        LD      A,(AUPUSH)
        OR      A
        RET     NZ
        LD      A,(MYSEAT)
        CP      4
        RET     NC
        LD      A,(AUHELD)
        LD      L,A
        LD      A,(FRAMES)
        SUB     L
        CP      60
        RET     NC
        PUSH    DE
        LD      A,(MYSEAT)
        ADD     A,AO_PUSH
        LD      E,A
        CALL    AGB             ; the server hasn't our release yet
        OR      A
        JR      NZ,AKP1
        LD      A,(MYSEAT)
        CALL    AUTGTP
        CALL    LDHL
        EX      DE,HL           ; DE = the server's target
        LD      A,(MYSEAT)
        CALL    AUPOSP
        CALL    LDHL            ; HL = the server's pos
        OR      A
        SBC     HL,DE
        JR      NZ,AKP1
        POP     DE              ; at rest: the server's
        RET
AKP1:   POP     DE
        LD      A,(MYSEAT)
        CALL    AUPOSP
        LD      (HL),C
        INC     HL
        LD      (HL),B
        LD      A,(MYSEAT)
        CALL    AUTGTP
        LD      (HL),E
        INC     HL
        LD      (HL),D
        RET
; LDHL: HL = (HL).
LDHL:   LD      A,(HL)
        INC     HL
        LD      H,(HL)
        LD      L,A
        RET
; ROLEOF: A = AUROLE[A].
ROLEOF: PUSH    HL
        LD      HL,AUROLE
        ADD     A,L
        LD      L,A
        JR      NC,ROF1
        INC     H
ROF1:   LD      A,(HL)
        POP     HL
        RET
; BITOF: A = 1 << A.
BITOF:  PUSH    BC
        LD      B,A
        INC     B
        LD      A,80H
BO1:    RLCA
        DJNZ    BO1
        POP     BC
        RET

; ---- prices and lines -------------------------------------------------------------
; PRICEY: A = the Atari line of price HL.
PRICEY: LD      DE,(AUBASE)
        OR      A
        SBC     HL,DE
        JR      C,PY2
        CALL    PYSTEP          ; d up
        CP      AUSTUP+1
        JR      C,PY1
        LD      A,AUSTUP
PY1:    ADD     A,A
        LD      B,A
        LD      A,AUYBASE
        SUB     B
        RET
PY2:    EX      DE,HL           ; d = base - w
        LD      HL,0
        OR      A
        SBC     HL,DE
        CALL    PYSTEP
        CP      AUSTDN+1
        JR      C,PY3
        LD      A,AUSTDN
PY3:    ADD     A,A
        ADD     A,AUYBASE
        RET
; PYSTEP: A = HL (/4 when the step is 4), at most 255.
PYSTEP: LD      A,(AUSTEPV)
        CP      4
        JR      NZ,PYS1
        SRL     H
        RR      L
        SRL     H
        RR      L
PYS1:   LD      A,H
        OR      A
        LD      A,L
        RET     Z
        LD      A,255
        RET

; AUDRAW: place the players (every 8 frames while the lines move, and after
; a state).
AUDRAW: XOR     A
AUD1:   LD      (AUI),A
        CALL    ROLEOF
        OR      A
        JP      Z,AUDHID
        LD      B,A
        LD      A,(AUSTAGE)
        CP      ST_LEAD
        JR      NC,AUD2
        LD      A,AUYSELL
        DEC     B
        DEC     B
        JR      Z,AUD3
        LD      A,AUYBOT
        JR      AUD3
AUD2:   LD      A,(AUI)
        CALL    AUPOSP
        CALL    LDHL
        CALL    PRICEY
AUD3:   LD      (AUJ),A         ; feet
        ; dealing: 16 frames on, 16 off
        LD      A,(AUI)
        CALL    BITOF
        LD      B,A
        LD      A,(AUBLINK)
        AND     B
        JR      Z,AUD4
        LD      A,(FRAMES)
        AND     16
        JR      NZ,AUDHID
AUD4:   LD      A,(AUI)
        LD      E,HP_SPEC
        CALL    PLB
        AND     7
        LD      B,A
        ADD     A,A
        ADD     A,A
        ADD     A,B
        ADD     A,A
        ADD     A,B             ; *11
        ADD     A,5
        LD      (SPA_FR),A
        LD      A,(AUI)         ; walking to its target: the other frame
        CALL    AUPOSP
        LD      E,(HL)
        INC     HL
        LD      D,(HL)
        LD      A,(AUI)
        CALL    AUTGTP
        CALL    LDHL
        OR      A
        SBC     HL,DE
        JR      Z,AUD5
        LD      A,(FRAMES)
        AND     8
        JR      Z,AUD5
        LD      HL,SPA_FR
        INC     (HL)
AUD5:   LD      A,SH_SPEC
        LD      (SPA_SH),A
        LD      A,(AUI)
        LD      E,A
        LD      D,0
        LD      HL,COLX
        ADD     HL,DE
        LD      L,(HL)
        LD      H,0
        LD      (SPA_X),HL
        LD      A,(AUJ)
        SUB     22
        LD      L,A
        LD      H,0
        LD      (SPA_Y),HL
        XOR     A
        LD      (SPA_FL),A
        LD      A,(AUI)
        CALL    PLC
        LD      (SPA_COL),A
        LD      A,(AUI)
        CALL    SPRSET
        JR      AUD9
AUDHID: LD      A,(AUI)
        CALL    SPRHIDE
AUD9:   LD      A,(AUI)
        INC     A
        CP      4
        JP      C,AUD1
        RET

; AUISR: every frame: the lines walk a step every 8 frames.
AUISR:  LD      A,(AUON)
        OR      A
        RET     Z
        LD      HL,AUTCK
        INC     (HL)
        LD      A,(HL)
        CP      8
        JR      NC,AUI1
        LD      A,(FRAMES)      ; the walk frames and blinking
        AND     7
        RET     NZ
        JP      AUDRAW
AUI1:   LD      (HL),0
        XOR     A
AUI2:   LD      (AUI),A
        CALL    AUTGTP
        CALL    LDHL
        EX      DE,HL           ; DE = target
        LD      A,(AUI)
        CALL    AUPOSP          ; HL = &pos
        PUSH    HL
        CALL    LDHL            ; HL = pos
        LD      A,(AUSTEPV)
        LD      C,A
        LD      B,0
        OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      Z,AUI5
        JR      NC,AUI3
        ADD     HL,BC           ; up to the target
        OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      C,AUI5
        EX      DE,HL
        JR      AUI5
AUI3:   OR      A               ; down to it
        SBC     HL,BC
        JR      NC,AUI4
        LD      HL,0
AUI4:   OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      NC,AUI5
        EX      DE,HL
AUI5:   EX      DE,HL           ; DE = the new pos
        POP     HL
        LD      (HL),E
        INC     HL
        LD      (HL),D
        ; land: a bid past 30 steps over the base scrolls the scale up
        LD      A,(AUKIND)
        CP      1
        JR      NZ,AUI6
        LD      A,(AUI)
        CALL    ROLEOF
        CP      RL_BUYER
        JR      NZ,AUI6
        LD      A,(AUSTEPV)     ; lim = step * 30
        LD      L,A
        LD      H,0
        LD      B,H
        LD      C,L
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL
        ADD     HL,HL           ; *16
        OR      A
        SBC     HL,BC           ; *15
        ADD     HL,HL           ; *30
        PUSH    HL
        LD      BC,(AUBASE)
        ADD     HL,BC           ; base + lim
        OR      A
        SBC     HL,DE
        POP     BC
        JR      NC,AUI6
        EX      DE,HL           ; base = pos - lim
        OR      A
        SBC     HL,BC
        LD      (AUBASE),HL
AUI6:   LD      A,(AUI)
        INC     A
        CP      4
        JR      C,AUI2
        JP      AUDRAW

; AULINES: the dotted price lines: the sellers' (lowest ask, or the store's
; price while it has stock) and the buyers' (highest bid, or the store's
; buying price / the bottom of the land scale).
AULINES: LD     A,(AUSTAGE)
        OR      A
        RET     Z               ; ST_SHOW
        LD      B,A
        LD      A,(AUKIND)
        CP      1
        JR      NZ,AUL1
        LD      A,B
        CP      ST_LEAD
        RET     C
AUL1:   LD      HL,0FFFFH
        LD      A,(AUKIND)
        CP      2
        JR      NZ,AUL2
        LD      A,(AUSTK)
        OR      A
        JR      Z,AUL2
        LD      E,AO_SSELL
        CALL    AGW
AUL2:   LD      (AUW),HL
        XOR     A
AUL3:   LD      (AUI),A
        CALL    ROLEOF
        CP      RL_SELLER
        JR      NZ,AUL4
        LD      A,(AUSTAGE)
        CP      ST_LEAD
        JR      C,AUL4
        LD      A,(AUI)
        CALL    AUPOSP
        CALL    LDHL
        LD      DE,(AUW)
        OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      NC,AUL4
        LD      (AUW),HL
AUL4:   LD      A,(AUI)
        INC     A
        CP      4
        JR      C,AUL3
        LD      A,255
        LD      (AUJ),A
        LD      HL,(AUW)
        LD      A,H
        AND     L
        INC     A
        JR      Z,AUL5
        CALL    PRICEY
        LD      (AUJ),A
AUL5:   LD      A,(AUKIND)      ; the buyers'
        CP      2
        LD      HL,(AUBASE)
        JR      NZ,AUL6
        LD      E,AO_SBUY
        CALL    AGW
AUL6:   LD      (AUW),HL
        XOR     A
AUL7:   LD      (AUI),A
        CALL    ROLEOF
        CP      RL_BUYER
        JR      NZ,AUL8
        LD      A,(AUSTAGE)
        CP      ST_LEAD
        JR      C,AUL8
        LD      A,(AUI)
        CALL    AUPOSP
        CALL    LDHL
        LD      DE,(AUW)
        OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      C,AUL8
        JR      Z,AUL8
        LD      (AUW),HL
AUL8:   LD      A,(AUI)
        INC     A
        CP      4
        JR      C,AUL7
        LD      HL,(AUW)
        CALL    PRICEY
        LD      (AUK),A
        ; never over the labels
        LD      A,(AUJ)
        CP      127
        JR      C,AUL9
        CP      255
        JR      Z,AUL9
        LD      A,255
        LD      (AUJ),A
AUL9:   LD      A,(AUK)
        CP      127
        JR      C,AUL10
        LD      A,255
        LD      (AUK),A
AUL10:  LD      A,(AULN1)
        LD      B,A
        LD      A,(AUJ)
        CP      B
        JR      NZ,AUL11
        LD      A,(AULN2)
        LD      B,A
        LD      A,(AUK)
        CP      B
        RET     Z
; The lines are XORed on and off (XHDOTS), as the walkers are: no lifting
; them, which blinked all four whenever a line moved. A line already on
; stays; two on one row are one.
AUL11:  LD      A,(AULN1)       ; the old lines off (unless kept)
        CALL    AUL1OFF
        LD      A,(AULN2)
        LD      B,A
        LD      A,(AULN1)
        CP      B
        LD      A,B
        CALL    NZ,AUL1OFF
        LD      A,(AUJ)         ; the new on (unless there)
        CALL    AUL1ON
        LD      A,(AUK)
        LD      B,A
        LD      A,(AUJ)
        CP      B
        LD      A,B
        CALL    NZ,AUL1ON
        LD      A,(AUJ)
        LD      (AULN1),A
        LD      A,(AUK)
        LD      (AULN2),A
        RET
AUL1OFF: CP     255
        RET     Z
        LD      B,A
        LD      A,(AUJ)
        CP      B
        RET     Z
        LD      A,(AUK)
        CP      B
        RET     Z
        JR      AUL1D
AUL1ON: CP      255
        RET     Z
        LD      B,A
        LD      A,(AULN1)
        CP      B
        RET     Z
        LD      A,(AULN2)
        CP      B
        RET     Z
AUL1D:  LD      C,B
        LD      A,CW
        LD      B,AULINX
        LD      D,AULINW+80H
        JP      HDOTS

; AULBLTICK: row 9, over each player's money: BUY / SELL while the player
; stands at its starting line, else the price it has moved to.
AULBLTICK: XOR  A
ALT1:   LD      (AUI),A
        CALL    ROLEOF
        LD      HL,1
        OR      A
        JR      Z,ALT3
        LD      HL,0
        LD      A,(AUSTAGE)
        CP      ST_LEAD
        JR      C,ALT3
        LD      A,(AUI)
        CALL    AUPOSP
        CALL    LDHL            ; w = pos
        LD      A,(AUI)
        CALL    ROLEOF
        CP      RL_BUYER
        JR      NZ,ALT2
        LD      E,AO_LO         ; a buyer at the bottom: BUY
        PUSH    HL
        CALL    AGW
        EX      DE,HL
        POP     HL
        OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      Z,ALT2Z
        JR      C,ALT2Z
        JR      ALT2B
ALT2:   LD      E,AO_HI         ; a seller at the top: SELL
        PUSH    HL
        CALL    AGW
        EX      DE,HL
        POP     HL
        OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      C,ALT2B
ALT2Z:  LD      HL,0
ALT2B:  LD      A,H
        OR      A
        JR      NZ,ALT3
        LD      A,L
        DEC     A
        JR      NZ,ALT3
        INC     L               ; 1 is "none": a price of 1 shows as 2
ALT3:   LD      A,(AUI)         ; unchanged?
        ADD     A,A
        LD      E,A
        LD      D,0
        PUSH    HL
        LD      HL,AULBL
        ADD     HL,DE
        POP     DE              ; DE = w
        LD      A,(HL)
        CP      E
        JR      NZ,ALT4
        INC     HL
        LD      A,(HL)
        DEC     HL
        CP      D
        JR      Z,ALT9
ALT4:   LD      (HL),E
        INC     HL
        LD      (HL),D
        LD      (AUW),DE
        LD      A,(AUI)         ; the field: 10 cells
        LD      B,10
        CALL    MULB4
        LD      E,A
        LD      D,ROWLBL
        LD      B,10
        CALL    CLRTXT
        LD      HL,(AUW)
        LD      A,H
        OR      L
        JR      NZ,ALT5
        LD      A,(AUI)         ; the label
        CALL    ROLEOF
        LD      B,W_BUY
        CP      RL_SELLER
        JR      NZ,ALT4A
        LD      B,W_SELL
ALT4A:  LD      A,(AUI)
        PUSH    BC
        LD      B,10
        CALL    MULB4
        POP     BC
        ADD     A,2
        LD      E,A
        LD      D,ROWLBL
        LD      A,(AUI)
        CALL    PLC
        LD      C,A
        LD      A,1
        LD      (SDMODE),A
        LD      A,K_WORD
        CALL    SDRAWM
        JR      ALT9
ALT5:   LD      A,H
        OR      A
        JR      NZ,ALT6
        LD      A,L
        CP      2
        JR      C,ALT9          ; 1: none
ALT6:   LD      A,(AUI)         ; a price, big
        LD      B,10
        CALL    MULB4
        LD      E,A
        LD      D,ROWLBL
        LD      B,5
        LD      A,(AUI)
        CALL    PLC
        LD      C,A
        CALL    AUBIGN
ALT9:   LD      A,(AUI)
        INC     A
        CP      4
        JP      C,ALT1
        RET

; AUBIGN: HL right-aligned in B big characters at row D, column E, colour C.
AUBIGN: PUSH    DE
        PUSH    BC
        CALL    NUMR
        POP     BC
        POP     DE
        LD      HL,LINBUF
        JP      BIGAT
; AUNUM: HL right-aligned in B cells at row D, column E, colour C.
AUNUM:  PUSH    DE
        PUSH    BC
        CALL    NUMR
        POP     BC
        POP     DE
        LD      HL,LINBUF
        JP      TXTAT

; MULB4: A = A * B. Preserves the rest but C.
MULB4:  PUSH    BC
        LD      C,A
        XOR     A
MB41:   ADD     A,C
        DJNZ    MB41
        POP     BC
        RET

AULABELS: LD    HL,AULBL
        LD      B,8
ALB1:   LD      (HL),0FFH
        INC     HL
        DJNZ    ALB1
        CALL    AULBLTICK
        LD      A,(AUKIND)
        CP      2
        RET     NZ
        ; the store's icons: selling above the blue one, buying under the
        ; orange one
        LD      A,0             ; PIC_STOREICON
        LD      B,136
        LD      C,32
        LD      E,CB
        CALL    APIC
        LD      A,0
        LD      B,4
        LD      C,112
        LD      E,CO
        CALL    APIC
        LD      E,AO_SSELL
        CALL    AGW
        LD      D,3
        LD      E,32
        LD      B,4
        LD      C,CW
        CALL    AUNUM
        LD      E,AO_SBUY
        CALL    AGW
        LD      D,8
        LD      E,0
        LD      B,4
        LD      C,CW
        JP      AUNUM

; AUMONEY: money and (goods) units, in ten-cell fields under the players.
AUMONEY: XOR    A
AUM1:   LD      (AUI),A
        LD      E,HP_MONEY
        CALL    PLW
        LD      A,(AUI)
        LD      B,10
        CALL    MULB4
        LD      E,A
        LD      D,ROWMON
        LD      A,(AUI)
        CALL    PLC
        LD      C,A
        LD      B,5
        CALL    AUBIGN
        LD      A,(AUKIND)
        CP      2
        JR      NZ,AUM2
        LD      A,(AUSTAGE)
        CP      ST_LEAD
        JR      C,AUM2
        LD      A,(AUGOOD)
        ADD     A,HP_GOODS
        LD      E,A
        LD      A,(AUI)
        CALL    PLB
        LD      L,A
        LD      H,0
        LD      A,(AUI)
        LD      B,10
        CALL    MULB4
        ADD     A,4
        LD      E,A
        LD      D,ROWUNIT
        LD      B,4
        LD      C,CW
        CALL    AUNUM
AUM2:   LD      A,(AUI)
        INC     A
        CP      4
        JR      C,AUM1
        RET

; AUTIMEBAR: the auction clock: a bar, a line for every tick left.
AUTIMEBAR: LD   A,CW
        LD      (BARCOL),A
        CALL    AUBARU
        JP      BARDRAW

; AUBARU: A = the clock's ticks left (0 when it isn't running).
AUBARU: LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        JR      Z,ATB1
        CP      ST_DECL
        LD      A,0
        RET     NZ
ATB1:   LD      E,AO_TICKS
        JP      AGB

; AUMESSAGE: the message row.
AUMESSAGE: LD   A,ROWMSG
        LD      (MSGROW),A
        LD      A,CW
        LD      (MSGCOL),A
        LD      C,255
        LD      A,(AUSTAGE)
        CP      ST_DECL
        JR      NZ,AMS1
        LD      A,(MYSEAT)      ; (a spectator isn't asked)
        CP      4
        LD      A,ST_DECL
        JR      NC,AMS1
        LD      C,M_DECLARE
AMS1:   CP      ST_DONE
        JR      NZ,AMS2
        LD      C,M_AUCOVER
        LD      A,(AUKIND)
        CP      1
        JR      NZ,AMS2
        LD      E,AO_WINNER
        CALL    AGB
        CP      4
        LD      C,M_SOLD
        JR      C,AMS2
        LD      C,M_NOBIDS
AMS2:   LD      A,(AUKIND)
        CP      2
        JR      NZ,AMS3
        LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        JR      NZ,AMS3
        LD      E,AO_FLAGS
        CALL    AGB
        LD      B,A
        AND     AF_COLL
        JR      Z,AMS2A
        LD      C,M_COLLUDE
AMS2A:  LD      A,B
        AND     AF_CRIT
        JR      Z,AMS3
        LD      C,M_CRITICAL
AMS3:   LD      A,(AUKIND)      ; land: the bottom of the scale
        CP      1
        JR      NZ,AMS5
        LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        JR      NZ,AMS5
        LD      A,(AUMSG)
        CP      C
        JR      NZ,AMS4
        LD      HL,(AUBASE)
        LD      DE,(AUSB)
        OR      A
        SBC     HL,DE
        RET     Z
AMS4:   LD      A,C
        LD      (AUMSG),A
        LD      HL,(AUBASE)
        LD      (AUSB),HL
        LD      B,M_BIDSTART
        LD      DE,0
        JP      MSGSHOW
AMS5:   LD      A,(AUMSG)
        CP      C
        RET     Z
        LD      A,C
        LD      (AUMSG),A
        CP      255
        JP      Z,MSGCLR
        LD      B,A
        LD      HL,0
        LD      D,H
        LD      E,L
        JP      MSGSHOW

; AULANDLINE: the land auction's price line (row 2): the opening price,
; then the high bid, then what it sold for.
AULANDLINE: LD  A,(AUKIND)
        CP      1
        RET     NZ
        LD      E,AO_START
        CALL    AGW
        LD      (AUW),HL
        LD      A,M_BIDSTART
        LD      (AUK),A
        XOR     A
ALL1:   LD      (AUI),A
        CALL    ROLEOF
        CP      RL_BUYER
        JR      NZ,ALL2
        LD      A,(AUI)
        CALL    AUPOSP
        CALL    LDHL
        LD      DE,(AUW)
        OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      C,ALL2
        LD      (AUW),HL
        LD      A,M_HIGHBID
        LD      (AUK),A
ALL2:   LD      A,(AUI)
        INC     A
        CP      4
        JR      C,ALL1
        LD      A,(AUSTAGE)
        CP      ST_DONE
        JR      NZ,ALL3
        LD      E,AO_PRICE
        CALL    AGW
        LD      A,H
        OR      L
        JR      Z,ALL3
        LD      (AUW),HL
        LD      A,M_SALEPRICE
        LD      (AUK),A
ALL3:   LD      HL,(AUW)
        LD      DE,(AUHB)
        OR      A
        SBC     HL,DE
        RET     Z
        LD      HL,(AUW)
        LD      (AUHB),HL
        LD      A,(AUK)
        LD      B,A
        LD      A,K_MSG
        LD      D,2
        LD      C,CW
        JP      SHTX

; AUBLINKERS: who is dealing blinks: in a land auction the high bidder (the
; winner once sold); in a goods auction the buyer and seller whose lines
; meet, or whoever meets the store's line.
AUBLINKERS: XOR A
        LD      (AUBLINK),A
        LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        RET     C
        LD      A,(AUKIND)
        CP      1
        JR      NZ,ABK3
        LD      A,(AUSTAGE)
        CP      ST_DONE
        JR      NZ,ABK1
        LD      E,AO_WINNER
        CALL    AGB
        CP      4
        RET     NC
        CALL    BITOF
        LD      (AUBLINK),A
        RET
ABK1:   LD      E,AO_START      ; the high bidder
        CALL    AGW
        CALL    ABHIGH
        LD      A,(AUJ)
        CP      4
        RET     NC
        CALL    BITOF
        LD      (AUBLINK),A
        RET
ABK3:   LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        RET     NZ
        LD      HL,0            ; the high buyer (AUJ, at AUW)
        CALL    ABHIGH
        LD      HL,0FFFFH       ; the low seller (AUK, at AULO)
        LD      (AULO),HL
        LD      A,255
        LD      (AUK),A
        XOR     A
ABK4:   LD      (AUI),A
        CALL    ROLEOF
        CP      RL_SELLER
        JR      NZ,ABK5
        LD      A,(AUI)
        CALL    AUPOSP
        CALL    LDHL
        LD      DE,(AULO)
        OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      Z,ABK4A
        JR      NC,ABK5
ABK4A:  LD      (AULO),HL
        LD      A,(AUI)
        LD      (AUK),A
ABK5:   LD      A,(AUI)
        INC     A
        CP      4
        JR      C,ABK4
        LD      A,(AUJ)         ; both, meeting?
        CP      4
        JR      NC,ABK6
        LD      A,(AUK)
        CP      4
        JR      NC,ABK6
        LD      HL,(AUW)
        LD      DE,(AULO)
        OR      A
        SBC     HL,DE
        JR      C,ABK6
        LD      A,(AUJ)
        CALL    BITOF
        LD      B,A
        LD      A,(AUK)
        CALL    BITOF
        OR      B
        LD      (AUBLINK),A
        RET
ABK6:   LD      A,(AUJ)         ; or the store's lines
        CP      4
        JR      NC,ABK7
        LD      E,AO_SSELL
        CALL    AGW
        EX      DE,HL
        LD      HL,(AUW)
        OR      A
        SBC     HL,DE
        JR      C,ABK7
        LD      A,(AUJ)
        CALL    BITOF
        LD      (AUBLINK),A
ABK7:   LD      A,(AUK)
        CP      4
        RET     NC
        LD      E,AO_SBUY
        CALL    AGW
        LD      DE,(AULO)
        OR      A
        SBC     HL,DE
        RET     C
        LD      A,(AUK)
        CALL    BITOF
        LD      B,A
        LD      A,(AUBLINK)
        OR      B
        LD      (AUBLINK),A
        RET

; ABHIGH: the buyer at or over HL, highest: AUJ (255 none), AUW.
ABHIGH: LD      (AUW),HL
        LD      A,255
        LD      (AUJ),A
        XOR     A
ABH1:   LD      (AUI),A
        CALL    ROLEOF
        CP      RL_BUYER
        JR      NZ,ABH2
        LD      A,(AUI)
        CALL    AUPOSP
        CALL    LDHL
        LD      DE,(AUW)
        OR      A
        SBC     HL,DE
        ADD     HL,DE
        JR      C,ABH2
        LD      (AUW),HL
        LD      A,(AUI)
        LD      (AUJ),A
ABH2:   LD      A,(AUI)
        INC     A
        CP      4
        JR      C,ABH1
        RET

; ---- the per-good status (stage 0, 561 jiffies): previous amount, usage,
; spoilage, production, then what the store holds; each player's bar stands
; over them.
; AUNEED: A = the units of the good seat A needs in round C: food by round
; (none after 12), energy one for each of its other M.U.L.E.s (HOT's map).
AUNEED: LD      B,A
        LD      A,(AUGOOD)
        OR      A
        JR      NZ,AN3
        LD      A,C             ; food
        CP      13
        LD      A,0
        RET     NC
        LD      A,C
        CP      8
        LD      A,5
        RET     NC
        LD      A,C
        CP      4
        LD      A,4
        RET     NC
        LD      A,3
        RET
AN3:    CP      1
        LD      A,0
        RET     NZ
        LD      C,0             ; energy
        LD      HL,HOWNER
        LD      DE,HMULE
AN4:    LD      A,(HL)
        CP      B
        JR      NZ,AN5
        LD      A,(DE)
        CP      4
        JR      NC,AN5
        CP      1
        JR      Z,AN5
        INC     C
AN5:    INC     HL
        INC     DE
        LD      A,L
        CP      (HOWNER+NPLOTS) AND 0FFH
        JR      NZ,AN4
        LD      A,C
        RET

; AUBARDRAW: seat A's bar (Atari: x col + 1, 8 wide, from BARBOT up).
AUBARDRAW: LD   (AUJ),A
        LD      E,A
        LD      D,0
        LD      HL,AUBAR
        ADD     HL,DE
        LD      A,(HL)
        LD      (AUK),A
        LD      HL,COLX
        ADD     HL,DE
        LD      A,(HL)
        INC     A
        LD      (AUI),A         ; (x)
        LD      B,A
        LD      C,BARBOT-BARMAX
        LD      A,(AUK)
        LD      E,A
        LD      A,BARMAX
        SUB     E
        LD      E,A
        LD      D,2
        LD      A,CK
        CALL    AFILL
        LD      A,(AUK)
        OR      A
        RET     Z
        LD      E,A
        LD      A,BARBOT
        SUB     E
        LD      C,A
        LD      A,(AUI)
        LD      B,A
        LD      A,(AUJ)
        CALL    PLC
        LD      D,2
        JP      AFILL

; AUSTTARGETS: each bar's height for the step shown (from the window: the
; production and spoilage are only there).
AUSTTARGETS: LD A,1
        LD      (MAPSRC),A
        XOR     A
AST1:   LD      (AUI),A
        LD      A,(AUGOOD)
        ADD     A,HP_GOODS
        LD      E,A
        LD      A,(AUI)
        CALL    PLB
        LD      (AUW),A         ; w = goods
        LD      A,(AUI)
        LD      E,PL_PROD
        CALL    WPLAYER
        LD      A,(AUGOOD)
        ADD     A,L
        LD      L,A
        JR      NC,AST1A
        INC     H
AST1A:  CALL    RXGETB
        LD      (AUW+1),A       ; c = production
        LD      DE,4
        ADD     HL,DE
        CALL    RXGETB
        LD      (AUJ),A         ; j = spoilage
        LD      A,(RND)
        LD      C,A
        LD      A,(AUI)
        CALL    AUNEED
        LD      (AUK),A         ; n
        ; previous = now - production + spoilage + usage
        LD      A,(AUW)
        LD      L,A
        LD      H,0
        LD      A,(AUJ)
        CALL    ADDA
        LD      A,(AUK)
        CALL    ADDA
        LD      A,(AUW+1)
        CALL    SUBA
        LD      A,(AUSHOWT)
        CP      1
        JR      NZ,AST2
        LD      A,(AUK)
        CALL    SUBA
AST2:   LD      A,(AUSHOWT)
        CP      2
        JR      NZ,AST3
        LD      A,(AUW)
        LD      L,A
        LD      H,0
        LD      A,(AUW+1)
        CALL    SUBA
AST3:   LD      A,(AUSHOWT)
        CP      3
        JR      C,AST4
        LD      A,(AUW)
        LD      L,A
        LD      H,0
AST4:   ADD     HL,HL           ; 4 lines a unit
        ADD     HL,HL
        LD      A,H
        OR      A
        LD      A,L
        JR      NZ,AST5
        CP      BARMAX+1
        JR      C,AST6
AST5:   LD      A,BARMAX
AST6:   LD      B,A
        LD      A,(AUI)
        LD      E,A
        LD      D,0
        LD      HL,AUBART
        ADD     HL,DE
        LD      (HL),B
        LD      A,(AUI)
        INC     A
        CP      4
        JP      C,AST1
        XOR     A
        LD      (MAPSRC),A
        RET
; ADDA: HL += A. SUBA: HL -= A, not under 0.
ADDA:   ADD     A,L
        LD      L,A
        RET     NC
        INC     H
        RET
SUBA:   LD      E,A
        LD      D,0
        OR      A
        SBC     HL,DE
        RET     NC
        LD      HL,0
        RET

; AUNEEDDRAW: once the store's stock is shown: what each player needs next
; round, a dotted line at that height by the bar and, under the player, how
; far short (-) or over (+).
AUNEEDDRAW: XOR A
AND1:   LD      (AUI),A
        LD      A,(RND)
        INC     A
        LD      C,A
        LD      A,(AUI)
        CALL    AUNEED
        OR      A
        JP      Z,AND9
        LD      (AUK),A         ; n
        LD      A,(AUGOOD)
        ADD     A,HP_GOODS
        LD      E,A
        LD      A,(AUI)
        CALL    PLB
        LD      (AUJ),A         ; w
        LD      A,(AUK)
        CP      21
        LD      A,BARBOT-BARMAX
        JR      NC,AND2
        LD      A,(AUK)
        ADD     A,A
        ADD     A,A
        LD      B,A
        LD      A,BARBOT
        SUB     B
AND2:   LD      (AUW),A         ; y
        LD      A,(AUI)
        LD      E,A
        LD      D,0
        LD      HL,COLX
        ADD     HL,DE
        LD      A,(HL)
        SUB     8
        RRCA
        RRCA
        AND     3FH
        LD      B,A
        PUSH    BC
        LD      A,(AUW)
        LD      C,A
        LD      D,2
        LD      A,CW
        CALL    HDOTS
        POP     BC
        LD      A,B
        ADD     A,5             ; (x + 12) / 4
        LD      B,A
        LD      A,(AUW)
        LD      C,A
        LD      D,2
        LD      A,CW
        CALL    HDOTS
        ; "+n" (white) / "-n" (orange)
        LD      A,(AUJ)
        LD      B,A
        LD      A,(AUK)
        SUB     B               ; n - w
        JR      C,AND3
        JR      Z,AND3
        LD      C,'-'
        LD      E,CO
        JR      AND4
AND3:   NEG                     ; w - n
        LD      C,'+'
        LD      E,CW
AND4:   PUSH    AF
        LD      A,C
        LD      (AUBUF),A
        LD      A,E
        LD      (AUW+1),A       ; the colour
        POP     AF
        LD      L,A
        LD      H,0
        CALL    NUMSTR
        LD      HL,LINBUF
        LD      DE,AUBUF+1
        LD      BC,6
        LDIR
        LD      A,(AUI)
        LD      B,10
        CALL    MULB4
        LD      (AUW),A
        LD      E,A
        LD      D,ROWLBL
        LD      B,10
        CALL    CLRTXT
        LD      A,(AUW)
        ADD     A,2
        LD      E,A
        LD      D,ROWLBL
        LD      A,(AUW+1)
        LD      C,A
        LD      HL,AUBUF
        CALL    BIGAT
AND9:   LD      A,(AUI)
        INC     A
        CP      4
        JP      C,AND1
        RET

AUSTSETUP: LD   A,RL_BUYER      ; the players standing at the bottom
        LD      HL,AUROLE
        LD      B,4
ASS1:   LD      (HL),A
        INC     HL
        DJNZ    ASS1
        XOR     A
        LD      (AUSTAGE),A
        LD      (AUBLINK),A
        CALL    AUDRAW
        LD      B,4
        LD      HL,AUROLE
        LD      E,AO_ROLE
ASS2:   CALL    AGB
        LD      (HL),A
        INC     HL
        INC     E
        DJNZ    ASS2
        XOR     A
        LD      HL,AUBAR
        LD      B,8
ASS3:   LD      (HL),A
        INC     HL
        DJNZ    ASS3
        LD      A,255
        LD      (AUSHOWT),A
        RET

AUSTTICK: CALL  PENOW
        LD      C,0
        LD      DE,127
        CALL    AUCMP
        LD      DE,250
        CALL    AUCMP
        LD      DE,373
        CALL    AUCMP
        LD      DE,496
        CALL    AUCMP
        LD      A,(AUSHOWT)
        CP      C
        JR      Z,AUT3
        LD      A,(STVALID)     ; the window has what the bars need
        OR      A
        JR      Z,AUT3
        LD      A,C
        LD      (AUSHOWT),A
        CALL    AUSTTARGETS
        LD      A,ROWMSG
        LD      (MSGROW),A
        LD      A,CW
        LD      (MSGCOL),A
        LD      A,(AUSHOWT)
        CP      4
        JR      NC,AUT2
        ADD     A,M_PREVIOUS
        LD      B,A
        LD      HL,0
        LD      D,H
        LD      E,L
        CALL    MSGSHOW
        JR      AUT3
AUT2:   LD      A,(AUSTK)
        LD      L,A
        LD      H,0
        LD      B,M_STOREHAS
        OR      A
        JR      NZ,AUT2A
        LD      B,M_STORENO
AUT2A:  LD      DE,0
        CALL    MSGSHOW
        CALL    AUNEEDDRAW
AUT3:   ; bars move a line every frame (the Atari's pixel every other)
        XOR     A
AUT4:   LD      (AUI),A
        LD      E,A
        LD      D,0
        LD      HL,AUBAR
        ADD     HL,DE
        LD      B,(HL)
        LD      HL,AUBART
        ADD     HL,DE
        LD      C,(HL)
        LD      A,B
        CP      C
        JR      Z,AUT7
        JR      NC,AUT5
        ADD     A,2
        CP      C
        JR      C,AUT6
        LD      A,C
        JR      AUT6
AUT5:   SUB     2
        JR      C,AUT5A
        CP      C
        JR      NC,AUT6
AUT5A:  LD      A,C
AUT6:   LD      HL,AUBAR
        ADD     HL,DE
        LD      (HL),A
        LD      A,(AUI)
        CALL    AUBARDRAW
AUT7:   LD      A,(AUI)
        INC     A
        CP      4
        JR      C,AUT4
        RET
; AUCMP: C += 1 if HL >= DE.
AUCMP:  PUSH    HL
        OR      A
        SBC     HL,DE
        POP     HL
        RET     C
        INC     C
        RET

; ---- screen setup ------------------------------------------------------------------
AUENTER: XOR    A
        LD      (ANIM),A
        LD      (AUON),A
        CALL    FXSTOP
        CALL    SPROFF
        CALL    CLRSCR
        LD      A,SCR_AUC
        LD      (SCRNOW),A
        LD      A,255
        LD      (AULN1),A
        LD      (AULN2),A
        LD      (AUSHOWT),A
        LD      (AUMSG),A
        LD      (BARLAST),A
        LD      HL,0FFFFH
        LD      (AUHB),HL
        LD      A,152
        LD      (BARX),A
        LD      A,32
        LD      (BARTOP),A
        LD      A,112
        LD      (BARH),A
        RET

; AUTITLE: "AUCTION #n" / "STATUS #n" over the good, or the land title.
AUTITLE: XOR    A
        CALL    CLRROW
        LD      A,1
        CALL    CLRROW
        LD      A,(AUKIND)
        CP      1
        JR      NZ,ATT1
        LD      A,K_TITLE       ; "LAND AUCTION #%"
        LD      C,T_LANDAUC
        JR      ATT2
ATT1:   LD      A,(AUSTAGE)
        LD      C,W_STATUS
        CP      ST_DECL+1
        JR      C,ATT1A
        LD      C,W_AUCTION
ATT1A:  LD      A,K_WORD
ATT2:   LD      DE,AUBUF
        LD      B,20
        CALL    STRGET
        LD      HL,AUBUF        ; the round where the '%' is, or at the end
ATT3:   LD      A,(HL)
        OR      A
        JR      Z,ATT4
        CP      '%'
        JR      Z,ATT4
        INC     HL
        JR      ATT3
ATT4:   PUSH    HL
        LD      A,(RND)
        LD      L,A
        LD      H,0
        CALL    NUMSTR
        POP     DE
        LD      HL,LINBUF
        LD      BC,4
        LDIR
        LD      HL,AUBUF
        LD      D,0
        LD      C,CO
        CALL    BIGCTR
        LD      A,(AUKIND)
        CP      1
        RET     Z
        LD      A,3
        LD      (SDMODE),A
        LD      A,(AUGOOD)
        LD      B,A
        LD      A,K_GOOD
        LD      D,1
        LD      C,CB
        JP      SDRAWM

AUSTBEGIN: CALL SPROFF         ; (the rows go: XOR sprites with them)
        LD      A,2
ASB1:   PUSH    AF
        CALL    CLRROW
        POP     AF
        INC     A
        CP      13
        JR      C,ASB1
        LD      A,255
        LD      (AULN1),A
        LD      (AULN2),A
        LD      (AUMSG),A
        LD      (BARLAST),A
        LD      HL,0FFFFH
        LD      (AUSB),HL
        LD      (AUHB),HL
        ; a new plot or good: the server has no push from us yet
        LD      A,(AUSTAGE)
        OR      A
        JR      NZ,ASB2
        LD      (AUPUSH),A
        LD      A,(AUKIND)
        CP      2
        JR      NZ,ASB2
        XOR     A
        LD      (AUON),A
        LD      (ANIM),A
        JP      AUSTSETUP
ASB2:   LD      A,(AUSTAGE)
        DEC     A               ; declaring (1), the lines about to go (2):
        CP      ST_LEAD         ; your hand to the stick
        CALL    C,SNDALRT
        LD      A,(AUSTAGE)
        CP      ST_DONE
        LD      A,SNAUCO
        CALL    Z,SNDPLAY
        JP      AULABELS

LAENTER: LD     A,255
        LD      (AUSTAGE),A
        XOR     A
        LD      (AUPUSH),A
        LD      (AUTRADE),A
        LD      (MAPDOTS),A
        RET

; GDENTER: declare and trading share the screen; a new good starts it
; afresh.
GDENTER: LD     A,(LASTPH)
        CP      PH_DECL
        JR      Z,GDE1
        CP      PH_TRADE
        JR      Z,GDE1
        XOR     A
        LD      (SCRNOW),A
GDE1:   LD      A,(PHS)
        CP      PH_DECL
        JR      NZ,GDE2
        XOR     A
        LD      (SCRNOW),A
GDE2:   LD      A,255
        LD      (AUSTAGE),A
        XOR     A
        LD      (AUPUSH),A
        LD      (AUTRADE),A
        RET

; ---- a new state in phases 2, 5 or 6 (auction_state) -----------------------------
AUSTATE: LD     E,AO_KIND
        CALL    AGB
        LD      (AUKIND),A
        LD      E,AO_STAGE
        CALL    AGB
        LD      (AUNEW),A
        LD      E,AO_GOOD
        CALL    AGB
        AND     3
        LD      (AUGOOD),A
        LD      E,AO_STEP
        CALL    AGB
        OR      A
        JR      NZ,AS1
        INC     A
AS1:    LD      (AUSTEPV),A
        LD      B,4
        LD      HL,AUROLE
        LD      E,AO_ROLE
AS2:    CALL    AGB
        LD      (HL),A
        INC     HL
        INC     E
        DJNZ    AS2
        LD      A,(AUGOOD)      ; the store's stock of it
        LD      E,A
        LD      D,0
        LD      HL,W_STORE
        ADD     HL,DE
        CALL    RXGETB
        LD      (AUSTK),A
        ; the land auction starts with the plot blinking on the map
        LD      A,(AUKIND)
        CP      1
        JR      NZ,AS5
        LD      A,(AUNEW)
        OR      A
        JR      NZ,AS5
        LD      A,(SCRNOW)
        CP      SCR_MAP
        JR      NZ,AS3
        LD      A,(AUSTAGE)
        OR      A
        JR      Z,AS4
AS3:    LD      A,(SCRNOW)
        CP      SCR_MAP
        CALL    NZ,MAPENTER
        LD      B,T_LANDAUC
        LD      A,(RND)
        LD      L,A
        LD      H,0
        CALL    SHTITLE
        LD      A,11
        LD      (MSGROW),A
        LD      A,CW
        LD      (MSGCOL),A
        LD      E,AO_START
        CALL    AGW
        LD      B,M_BIDSTART
        LD      DE,0
        CALL    MSGSHOW
AS4:    LD      A,(AUNEW)
        LD      (AUSTAGE),A
        LD      E,AO_PLOT
        CALL    AGB
        LD      (MAPBLK),A
        RET
AS5:    LD      A,(MAPBLK)      ; the plot off the blink
        CP      NPLOTS
        JR      NC,AS6
        LD      B,A
        LD      A,(SCRNOW)
        CP      SCR_MAP
        JR      NZ,AS6
        LD      A,NOBODY
        LD      (MAPBLK),A
        LD      A,B
        CALL    MAPPLOT
AS6:    LD      A,NOBODY
        LD      (MAPBLK),A
        LD      A,(SCRNOW)
        CP      SCR_AUC
        JR      NZ,AS7
        LD      A,(AUNEW)
        LD      B,A
        LD      A,(AUSTAGE)
        CP      B
        JR      Z,AS8
AS7:    LD      A,(SCRNOW)
        CP      SCR_AUC
        CALL    NZ,AUENTER
        LD      A,(AUNEW)
        LD      (AUSTAGE),A
        CALL    AUTITLE
        CALL    AUSTBEGIN
AS8:    ; the scale's base: land scrolls up with the bidding (AUISR), so the
        ; base only resets for a new plot
        LD      A,(AUKIND)
        CP      1
        JR      NZ,AS9
        LD      E,AO_START
        CALL    AGW
        LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        JR      C,AS8A
        LD      DE,(AUBASE)
        EX      DE,HL
        OR      A
        SBC     HL,DE
        JR      NC,AS10
        EX      DE,HL
AS8A:   LD      (AUBASE),HL
        JR      AS10
AS9:    LD      E,AO_SBUY
        CALL    AGW
        LD      (AUBASE),HL
AS10:   LD      A,(MYSEAT)      ; ours before the snap (AUKEEP)
        AND     3
        LD      C,A
        CALL    AUPOSP
        CALL    LDHL
        PUSH    HL              ; our pos
        LD      A,C
        CALL    AUTGTP
        CALL    LDHL
        PUSH    HL              ; our target
        LD      B,8             ; pos and target: snapped
        LD      HL,AUTGT
        LD      E,AO_TGT
AS11:   CALL    AGB
        LD      (HL),A
        INC     HL
        INC     E
        DJNZ    AS11
        LD      B,8
        LD      HL,AUPOS
        LD      E,AO_POS
AS12:   CALL    AGB
        LD      (HL),A
        INC     HL
        INC     E
        DJNZ    AS12
        POP     DE              ; our target
        POP     BC              ; our pos
        CALL    AUKEEP
        LD      A,(AUSTAGE)
        CP      ST_LEAD
        JR      C,AS13
        LD      A,1
        LD      (AUON),A
        LD      A,AN_AUC
        LD      (ANIM),A
AS13:   LD      A,(AUKIND)
        CP      2
        JR      NZ,AS14
        LD      A,(AUSTAGE)
        OR      A
        RET     Z
AS14:   CALL    AUMONEY
        CALL    AULINES
        CALL    AUTIMEBAR
        CALL    AUMESSAGE
        CALL    AULANDLINE
        CALL    AUBLINKERS
        CALL    AUDRAW
        LD      E,AO_TRADED
        CALL    AGB
        LD      B,A
        LD      A,(AUTRADE)
        CP      B
        RET     Z
        LD      A,B
        LD      (AUTRADE),A
        RET     NC              ; (more traded than shown: a sound)
        LD      A,SNTRADE
        JP      SNDPLAY

; AUSTICK: tell the server when our stick changes (up, down, released).
; It is sent before the lines are released too; the push carries AUTRADE.
AUSTICK: LD     A,(INDIR)
        LD      C,0
        BIT     2,A
        JR      Z,AK1
        LD      C,1
AK1:    BIT     0,A
        JR      Z,AK2
        LD      C,2
AK2:    LD      A,(AUPUSH)
        CP      C
        RET     Z
        LD      A,(ACT)
        OR      A
        RET     NZ
        LD      A,C
        LD      (AUPUSH),A
        LD      L,C
        LD      H,0
        LD      A,C             ; released ...
        OR      A
        JR      NZ,AK0A
        LD      A,(AUKIND)      ; ... in a goods auction under way: the line
        CP      2               ; stays where we saw it stop (A_HOLD, with
        JR      NZ,AK0A         ; our price; the server's line is behind)
        LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        JR      NZ,AK0A
        LD      A,(FRAMES)
        LD      (AUHELD),A
        LD      A,(MYSEAT)
        CALL    AUPOSP
        CALL    LDHL
        LD      A,A_HOLD
        JR      AK0
AK0A:   LD      A,A_PUSH
AK0:    CALL    ACTSEND
        LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        RET     NZ
        ; move our own line at once; the server confirms
        LD      A,(MYSEAT)
        CALL    AUTGTP
        PUSH    HL
        LD      A,(AUPUSH)
        CP      1
        JR      NZ,AK4
        LD      A,(AUKIND)
        CP      1
        JR      NZ,AK3
        LD      A,(MYSEAT)      ; land: as high as our money goes
        LD      E,HP_MONEY
        CALL    PLW
        LD      A,L
        AND     0FCH
        LD      L,A
        JR      AK7
AK3:    LD      E,AO_HI
        CALL    AGW
        JR      AK7
AK4:    CP      2               ; down, or (a land buyer) letting go
        JR      Z,AK5
        LD      A,(AUKIND)
        CP      1
        JR      NZ,AK6
        LD      A,(MYSEAT)
        CALL    ROLEOF
        CP      RL_BUYER
        JR      NZ,AK6
AK5:    LD      E,AO_LO
        CALL    AGW
        JR      AK7
AK6:    LD      A,(MYSEAT)      ; released: stay
        CALL    AUPOSP
        CALL    LDHL
AK7:    EX      DE,HL
        POP     HL
        LD      (HL),E
        INC     HL
        LD      (HL),D
        RET

AUTICK: CALL    AUBARU          ; the clock's tick, every frame
        CALL    FARCALL
        DB      PGMAP
        DW      XBARTICK
        LD      A,(AUKIND)
        CP      1
        JR      NZ,AT3
        LD      A,(AUSTAGE)
        OR      A
        JR      NZ,AT3
        ; the plot for sale blinks on the map (16 frames on, 16 off); the
        ; button skips the wait
        LD      A,(SCRNOW)
        CP      SCR_MAP
        JR      NZ,AT2
        LD      A,(MAPBLK)
        CP      NPLOTS
        JR      NC,AT2
        LD      A,(FRAMES)
        RRCA
        RRCA
        RRCA
        RRCA
        AND     1
        LD      B,A
        LD      A,(MAPBLON)
        CP      B
        JR      Z,AT2
        LD      A,B
        LD      (MAPBLON),A
        LD      A,(MAPBLK)
        CALL    MAPPLOT
AT2:    CALL    MSGTICK
        LD      A,(MYSEAT)
        CP      4
        RET     NC
        LD      A,(EVBTN)
        OR      A
        JP      Z,AUSTICK
        LD      A,A_READY
        LD      HL,0
        CALL    ACTSEND
        JP      AUSTICK
AT3:    LD      A,(SCRNOW)
        CP      SCR_AUC
        RET     NZ
        CALL    MSGTICK
        LD      A,(AUKIND)
        CP      2
        JR      NZ,AT4
        LD      A,(AUSTAGE)
        OR      A
        JP      Z,AUSTTICK
AT4:    CALL    AULINES
        CALL    AULBLTICK
        LD      A,(AUSTAGE)
        CP      ST_DECL
        JR      NZ,AT9
        LD      A,(MYSEAT)
        CP      4
        JR      NC,AT9
        ; push the stick to declare: up = seller, down = buyer
        CALL    ROLEOF
        LD      B,A
        LD      A,(INDIR)
        BIT     2,A
        JR      Z,AT5
        LD      A,B
        CP      RL_SELLER
        JR      Z,AT5
        LD      C,RL_SELLER
        LD      L,2
        JR      AT6
AT5:    LD      A,(INDIR)
        BIT     0,A
        JR      Z,AT7
        LD      A,B
        CP      RL_BUYER
        JR      Z,AT7
        LD      C,RL_BUYER
        LD      L,1
AT6:    PUSH    BC
        CALL    ATDECL
        POP     BC
        LD      A,SNSEL
        CALL    SNDPLAY
AT7:    LD      A,(EVKEY)       ; 0: sit this good out
        CP      '0'
        JR      NZ,AT8
        LD      A,B
        OR      A
        JR      NZ,AT7A
        LD      A,(MYSEAT)
        LD      E,HP_FLAG
        CALL    PLB
        AND     4               ; PF_DECLARED
        JR      NZ,AT8
AT7A:   LD      C,RL_NONE
        LD      L,0
        CALL    ATDECL
AT8:    JP      AUDRAW
AT9:    LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        JR      Z,AT10
        CP      ST_LEAD
        RET     NZ
AT10:   LD      A,(MYSEAT)
        CP      4
        RET     NC
        CALL    AUSTICK
        LD      A,(EVBTN)
        OR      A
        RET     Z
        LD      A,(AUKIND)
        CP      2
        RET     NZ
        LD      A,(AUSTAGE)
        CP      ST_ACTIVE
        RET     NZ
        LD      A,A_COLL
        LD      HL,0
        JP      ACTSEND

; ATDECL: our role C, declared to the server as L (0 out, 1 buy, 2 sell).
ATDECL: LD      A,(MYSEAT)
        LD      E,A
        LD      D,0
        LD      H,D
        PUSH    HL
        LD      HL,AUROLE
        ADD     HL,DE
        LD      (HL),C
        POP     HL
        LD      A,A_DECL
        CALL    ACTSEND
        JP      AULABELS
