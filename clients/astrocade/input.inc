; input.inc -- the hand controller and the keypad (input.h). Resident.
;
; INPOLL runs once a frame. Directions are levels (INDIR, the
; Intellivision disc's bits the game was written against); the trigger and
; the keys latch until INTAKE moves them into EVBTN / EVKEY, so a press made
; while the game waits on the network is not lost.
;
; Player 1's handle is port 10H: U/D/L/R/trigger in bits 0-4, active high.
; The keypad is four column ports, 14H (right) .. 17H (left), bits 0-5
; bottom key first (decoded from MAME's astrohome.cpp, as netcat does):
;   = ends the turn (KEY_END), C leaves (KEY_LEAVE; in a game after asking,
;   status.inc LVASK), CE rubs out (KEY_BACK),
;   the digits are themselves (0 sits a good out), MR mutes.

DIRDN   EQU     1
DIRRT   EQU     2
DIRUP   EQU     4
DIRLT   EQU     8

KEYEND  EQU     13
KEYLV   EQU     27
KEYBK   EQU     8
KEYMUTE EQU     1               ; handled here, never reaches the game

INPOLL: LD      A,(INPHK)
        OR      A
        JR      Z,INP1
        LD      A,(INPHKD)      ; the harness drives
        LD      (INDIR),A
        LD      A,(INPHKB)
        LD      B,A
        LD      A,(INPHKK)
        OR      A
        JR      Z,INP4
        LD      (LKEY),A
        XOR     A
        LD      (INPHKK),A
        JR      INP4
INP1:   IN      A,(SW0)
        LD      C,A
        XOR     A
        BIT     0,C
        JR      Z,$+4
        OR      DIRUP
        BIT     1,C
        JR      Z,$+4
        OR      DIRDN
        BIT     2,C
        JR      Z,$+4
        OR      DIRLT
        BIT     3,C
        JR      Z,$+4
        OR      DIRRT
        LD      (INDIR),A
        LD      A,C
        AND     10H
        LD      B,A
        CALL    KEYRAW          ; A = key, 0 none
        LD      HL,KPPREV
        CP      (HL)
        JR      Z,INP4
        LD      (HL),A
        OR      A
        JR      Z,INP4
        CP      KEYMUTE
        JR      NZ,INP3
        CALL    SNDMUTE
        JR      INP4
INP3:   LD      (LKEY),A
INP4:   LD      A,B             ; B = button level
        OR      A
        JR      Z,INP5
        LD      A,1
INP5:   LD      (INBTN),A
        LD      HL,BTPREV
        CP      (HL)
        LD      (HL),A
        RET     Z
        OR      A
        RET     Z
        LD      (LBTN),A
        RET

INTAKE: LD      HL,LBTN
        LD      A,(HL)
        LD      (HL),0
        LD      (EVBTN),A
        LD      HL,LKEY
        LD      A,(HL)
        LD      (HL),0
        LD      (EVKEY),A
        RET

; KEYRAW: A = the key held on the keypad now (its code, KPTAB), or 0.
; Clobbers A,C,D,E,H,L.
KEYRAW: LD      HL,KPTAB
        LD      C,KEY0
KRCOL:  IN      A,(C)
        LD      D,A
        LD      E,6
KRBIT:  SRL     D
        JR      NC,KRB2
        LD      A,(HL)
        OR      A
        RET     NZ
KRB2:   INC     HL
        DEC     E
        JR      NZ,KRBIT
        INC     C
        LD      A,C
        CP      KEY3+1
        JR      NZ,KRCOL
        XOR     A
        RET

; Per column 14H..17H, bit 0 (bottom key) first.
KPTAB:  DB      0,0,0,0,0,KEYEND                ; % / x - + =
        DB      0,0,'9','6','3',0               ; v CH 9 6 3 .
        DB      0,0,'8','5','2','0'             ; ^ MS 8 5 2 0
        DB      KEYLV,KEYMUTE,'7','4','1',KEYBK ; C MR 7 4 1 CE
