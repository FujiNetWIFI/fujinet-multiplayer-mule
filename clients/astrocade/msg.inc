; msg.inc -- the strings and the message line (text.c). Resident.
;
; Every string lives in the two string pages (gen/stra.inc, gen/strb.inc,
; from strings.c). Resident code maps the page while it reads, and maps the
; caller's page back.
;
; A message is never expanded into RAM (the C client's txt[128]): MSGWALK
; walks it -- a RAM prefix (a player's name), then the string, a '%'
; replaced by MSGV1 then MSGV2 -- straight onto the screen, skipping and
; clipping as the caller asks. A message fits one row centred, is
; word-wrapped onto two, or (longer still) scrolls in from the right along
; the second row, a character every 4 frames, as on the Atari.

K_TITLE EQU     0
K_MSG   EQU     1
K_WORD  EQU     2
K_GOOD  EQU     3
K_SPEC  EQU     4
K_PEV   EQU     5
K_RATE  EQU     6

; message ids (ids.h)
M_LANDING EQU   1
M_DEPART EQU    2
M_SELECT EQU    3
M_NONEW EQU     4
M_START EQU     5
M_COMP  EQU     6
M_TURN  EQU     7
M_TIMEOUT EQU   8
M_GAMBLE EQU    9
M_PRESSALL EQU  10
M_FOODSHORT EQU 11
M_EVENT EQU     12
M_PREVIOUS EQU  21
M_USAGE EQU     22
M_SPOIL EQU     23
M_PRODUCTION EQU 24
M_STOREHAS EQU  25
M_STORENO EQU   26
M_DECLARE EQU   27
M_AUCOVER EQU   28
M_CRITICAL EQU  29
M_SELLERDRY EQU 30
M_STOREDRY EQU  31
M_TRADED EQU    32
M_SOLD  EQU     33
M_NOBIDS EQU    34
M_BIDSTART EQU  35
M_HIGHBID EQU   36
M_SALEPRICE EQU 37
M_MONEY EQU     38
M_NOMONEY EQU   39
M_NOMULES EQU   40
M_ASSAY EQU     41
M_FORSALE EQU   46
M_NOTFORSALE EQU 47
M_RANAWAY EQU   48
M_CAUGHT EQU    49
M_SPECTATE EQU  50
M_WAITING EQU   51
M_READY EQU     52
M_COLLUDE EQU   53
M_LANDOFFICE EQU 54
M_NOSAMPLE EQU  55
M_OUTFIT EQU    56
M_SHIPBACK EQU  57
M_ENDTURN EQU   58
M_LOBBYHELP EQU 59
M_LEAVEHELP EQU 60
M_LEAVEASK EQU 61

T_LANDGRANT EQU 0
T_LANDAUC EQU   1
T_DEVEL EQU     2
T_PROD  EQU     3
T_TRANSPORT EQU 4
T_EVENT EQU     5

W_SUMMARY EQU   0
W_MONEY EQU     1
W_LAND  EQU     2
W_GOODS EQU     3
W_TOTAL EQU     4
W_COLONY EQU    5
W_FOUNDER EQU   6
W_AUCTION EQU   7
W_STATUS EQU    8
W_SELL  EQU     9
W_BUY   EQU     10
W_UNITS EQU     11
W_TRADED EQU    12
W_MULE  EQU     13
W_TOURN EQU     14
W_PRESSFOR EQU  15
W_STARS EQU     16
W_COMPUTER EQU  17
W_OPEN  EQU     18
W_OVERALL EQU   19

MSGROWS EQU     13              ; text rows on the screen

; STRLOC: A = kind (K_*), C = id -> B = page, HL = the string.
; Preserves DE.
STRLOC: PUSH    DE
        ADD     A,A
        ADD     A,A
        LD      L,A
        LD      H,0
        LD      DE,STRKIND
        ADD     HL,DE
        LD      B,(HL)          ; the page
        INC     HL
        LD      E,(HL)          ; its table
        INC     HL
        LD      D,(HL)
        LD      A,(CURPG)
        PUSH    AF
        LD      A,B
        CALL    SELPG
        LD      L,C
        LD      H,0
        ADD     HL,HL
        ADD     HL,DE
        LD      A,(HL)
        INC     HL
        LD      H,(HL)
        LD      L,A
        POP     AF
        CALL    SELPG
        POP     DE
        RET

STRKIND: DB     PGSTRA
        DW      STT
        DB      0
        DB      PGSTRA
        DW      STM
        DB      0
        DB      PGSTRA
        DW      STW
        DB      0
        DB      PGSTRA
        DW      STG
        DB      0
        DB      PGSTRA
        DW      STS
        DB      0
        DB      PGSTRB
        DW      STP
        DB      0
        DB      PGSTRB
        DW      STR
        DB      0

; ---- the walker -------------------------------------------------------------
; MWALK: walk the message (MSGPFX, then MSGPG:MSGADR with MSGV1/MSGV2),
; skipping MSGSKP characters and then drawing up to MSGMAX at the shadow-DE
; text position (TXTPOS, colours set). MSGK = the characters it has; MSGCUT
; = the last space at index 1..40. MSGMAX 0: count only. Clobbers all.
MWALK:  XOR     A
        LD      (MSGK),A
        LD      (MSGVN),A
        LD      (MSGCUT),A
        LD      A,(CURPG)
        PUSH    AF
        LD      HL,(MSGPFX)
        LD      A,H
        OR      L
        CALL    NZ,MWSTR
        LD      A,(MSGPG)
        CALL    SELPG
        LD      HL,(MSGADR)
        CALL    MWSTR
        POP     AF
        JP      SELPG

MWSTR:  LD      A,(HL)
        OR      A
        RET     Z
        INC     HL
        CP      '%'
        JR      NZ,MWS2
        PUSH    HL
        LD      HL,MSGVN
        LD      A,(HL)
        INC     (HL)
        OR      A
        LD      HL,(MSGV1)
        JR      Z,MWS1
        LD      HL,(MSGV2)
MWS1:   CALL    NUMSTR
        LD      HL,LINBUF
        CALL    MWSTR           ; the digits (no '%' among them)
        POP     HL
        JR      MWSTR
MWS2:   CALL    MWCH
        JR      MWSTR

; MWCH: one character A through the walker. Preserves HL.
MWCH:   PUSH    HL
        LD      HL,MSGK
        LD      B,(HL)
        INC     (HL)
        CP      ' '
        JR      NZ,MWC1
        LD      C,A
        LD      A,B
        OR      A
        JR      Z,MWC0
        CP      41
        JR      NC,MWC0
        LD      (MSGCUT),A
MWC0:   LD      A,C
MWC1:   LD      C,A
        LD      HL,MSGSKP
        LD      A,(HL)
        OR      A
        JR      Z,MWC2
        DEC     (HL)
        POP     HL
        RET
MWC2:   LD      HL,MSGMAX
        LD      A,(HL)
        OR      A
        JR      Z,MWC9
        DEC     (HL)
        LD      A,C
        CALL    GLYPHC
MWC9:   POP     HL
        RET


; ---- the wrappers: the message code lives in the string page -------------
MSGSHOW: CALL   FARCALL
        DB      PGSTRA
        DW      XMSGSHOW
        RET
MSGSTART: CALL  FARCALL
        DB      PGSTRA
        DW      XMSGSTART
        RET
MSGTICK: LD     A,(MSGSCR)
        OR      A
        RET     Z
        CALL    FARCALL
        DB      PGSTRA
        DW      XMSGTICK
        RET
MSGCLR: CALL    FARCALL
        DB      PGSTRA
        DW      XMSGCLR
        RET
SHTITLE: CALL   FARCALL
        DB      PGSTRA
        DW      XSHTITLE
        RET
SDRAW:  CALL    FARCALL
        DB      PGSTRA
        DW      XSDRAW
        RET
SDRAWM: CALL    FARCALL
        DB      PGSTRA
        DW      XSDRAWM
        RET

; STRGET: string kind A, id C into DE (NUL-terminated, at most B bytes
; with the NUL). Clobbers all.
STRGET: PUSH    DE
        PUSH    BC
        CALL    STRLOC          ; B = page, HL = string
        LD      A,(CURPG)
        LD      C,A
        LD      A,B
        POP     DE              ; D = max (was B)
        LD      E,C             ; E = the caller's page
        POP     BC              ; BC = dest
        PUSH    DE
        CALL    SELPG
        POP     DE
        PUSH    DE
SG1:    DEC     D
        JR      Z,SG2
        LD      A,(HL)
        LD      (BC),A
        OR      A
        JR      Z,SG3
        INC     HL
        INC     BC
        JR      SG1
SG2:    XOR     A
        LD      (BC),A
SG3:    POP     DE
        LD      A,E
        JP      SELPG
