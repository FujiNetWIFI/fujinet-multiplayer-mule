; dev2b.inc -- page 6's own copies of dev1.inc's little helpers.

D2MSG0: LD      HL,0
        LD      D,H
        LD      E,L
        JP      MSGSHOW

; D2PLB: seat A's player byte E (HP_*), window while MAPSRC.
D2PLB:  PUSH    HL
        PUSH    DE
        LD      D,A
        LD      A,(MAPSRC)
        OR      A
        LD      A,D
        JR      Z,D2B1
        PUSH    AF
        LD      A,E
        ADD     A,9
        LD      E,A
        POP     AF
        CALL    WPLAYER
        CALL    RXGETB
        JR      D2B2
D2B1:   CALL    HPLAYER
        LD      A,E
        ADD     A,L
        LD      L,A
        JR      NC,D2B1A
        INC     H
D2B1A:  LD      A,(HL)
D2B2:   POP     DE
        POP     HL
        RET
D2PLC:  PUSH    HL
        PUSH    DE
        LD      E,HP_COL
        CALL    D2PLB
        AND     3
        LD      E,A
        LD      D,0
        LD      HL,PLCOLT
        ADD     HL,DE
        LD      A,(HL)
        POP     DE
        POP     HL
        RET
