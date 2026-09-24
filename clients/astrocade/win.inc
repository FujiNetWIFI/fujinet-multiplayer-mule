; win.inc -- the state's layout and the reply window. Resident.
;
; The request engine is in the net page (netpg.inc); what stays resident is
; what every page needs: the offsets, reading the window, queueing actions.
; ---- the state's layout (clients/mule_proto.h) ----------------------------
STSIZE  EQU     550
W_PHASE EQU     3
W_ROUND EQU     4
W_SEAT  EQU     5
W_ERR   EQU     6
W_T     EQU     8
W_ELAP  EQU     10
W_CDOWN EQU     12
W_RESLT EQU     14
W_PL    EQU     16              ; + 29 * seat
PLSZ    EQU     29
PL_NAME EQU     0
PL_SPEC EQU     9
PL_COL  EQU     10
PL_FLAG EQU     11
PL_MONEY EQU    12
PL_GOODS EQU    14
PL_SCORE EQU    18
PL_RANK EQU     20
PL_PROD EQU     21
PL_SPOIL EQU    25
W_TERR  EQU     132
W_OWNER EQU     177
W_MULES EQU     222
W_CRYS  EQU     267
W_PROD  EQU     312
W_STORE EQU     357             ; stock[4] price[4] buy[4] sell[4] mules mulePrice
W_GRANT EQU     388
W_AUC   EQU     396
W_TURN  EQU     454
W_COL   EQU     540

; HOT player bytes (a copy of player bytes 9..20)
HP_SPEC EQU     0
HP_COL  EQU     1
HP_FLAG EQU     2
HP_MONEY EQU    3
HP_GOODS EQU    5
HP_SCORE EQU    9
HP_RANK EQU     11

PH_LOBBY EQU    0
PH_GRANT EQU    1
PH_LAUC EQU     2
PH_DEVEL EQU    3
PH_PROD EQU     4
PH_DECL EQU     5
PH_TRADE EQU    6
PH_SUMM EQU     7
PH_OVER EQU     8

PF_AI   EQU     1
PF_READY EQU    2

ERR_SEAT EQU    12
ERR_TFULL EQU   14
NPLOTS  EQU     45
STOREP  EQU     22
NOBODY  EQU     255

; actions (mule.h ACT_*): the route table's index
A_NONE  EQU     0
A_STATE EQU     1
A_READY EQU     2
A_UNRDY EQU     3
A_SPEC  EQU     4
A_CLAIM EQU     5
A_PUSH  EQU     6
A_DECL  EQU     7
A_COLL  EQU     8
A_BEGIN EQU     9
A_BUYM  EQU     10
A_OUTFIT EQU    11
A_INSTAL EQU    12
A_RETURN EQU    13
A_ASSAY EQU     14
A_SELL  EQU     15
A_UNSELL EQU    16
A_WAMPUS EQU    17
A_PUB   EQU     18
A_END   EQU     19
A_LEAVE EQU     20
A_TABLES EQU    21              ; not an action: the table list (boot)

NTMOUT  EQU     240             ; frames without progress: give up

; ---- reading the window -----------------------------------------------------
; RXGETB: A = reply byte at offset HL. Preserves HL, BC, DE. (In the
; offline test mode the test page makes the state up: TSTGETB.)
RXGETB: LD      A,(TESTMD)
        OR      A
        JR      Z,RXG0
        PUSH    IX
        CALL    FARCALL
        DB      PGTEST
        DW      TSTGETB
        POP     IX
        RET
RXG0:   PUSH    BC
        PUSH    DE
        PUSH    HL
        LD      A,H
        LD      HL,CURSLC
        CP      (HL)
        JR      Z,RXG1
        LD      (HL),A
        CALL    FNSLICE
RXG1:   POP     HL
        PUSH    HL
        LD      D,3BH
        LD      E,L
        LD      A,(DE)
        POP     HL
        POP     DE
        POP     BC
        RET

; RXGETW: DE = the word at offset HL. Preserves HL, BC.
RXGETW: CALL    RXGETB
        LD      E,A
        INC     HL
        CALL    RXGETB
        LD      D,A
        DEC     HL
        RET

; RXCOPY: BC bytes from offset HL to DE. Returns DE past them.
; Clobbers A,BC,HL.
RXCOPY: LD      A,B
        OR      C
        RET     Z
        LD      A,(TESTMD)
        OR      A
        JR      Z,RXCW
RXCT:   CALL    RXGETB          ; test mode: a byte at a time
        LD      (DE),A
        INC     HL
        INC     DE
        DEC     BC
        LD      A,B
        OR      C
        JR      NZ,RXCT
        RET
RXCW:   PUSH    DE
        LD      A,H
        LD      DE,CURSLC
        EX      DE,HL
        CP      (HL)
        EX      DE,HL
        JR      Z,RXC1
        LD      (CURSLC),A
        PUSH    HL
        PUSH    BC
        CALL    FNSLICE
        POP     BC
        POP     HL
RXC1:   POP     DE
        PUSH    HL              ; the offset
        LD      H,3BH           ; the window
RXC2:   LDI
        JP      PO,RXC9         ; BC ran out
        LD      A,L
        OR      A
        JR      NZ,RXC2
        POP     HL              ; crossed into the next slice
        INC     H
        LD      L,0
        JR      RXCOPY
RXC9:   POP     HL
        RET

; ACTSEND: queue action A with argument HL; poll now.
ACTSEND: LD     (ACT),A
        LD      (ACTARG),HL
        XOR     A
        LD      (POLLW),A
        RET

