; input.inc -- the hand controller (input.h). Fixed.
;
; OS7's POLLER reads controller 1 in the frame interrupt into CTLDAT (the
; header points it there); INPOLL turns that into the game's input once a
; frame. Directions are levels (INDIR, the Intellivision disc's bits the
; game was written against); the button and the keys latch until INTAKE
; moves them into EVBTN / EVKEY, so a press made while the game waits on
; the network is not lost.
;
;   stick       walks;   either fire button is the button
;   #           ends the turn, finishes the name (KEY_END)
;   *           leaves the table (asks first in a game); rubs out a letter
;               while a name is entered (INNAME)
;   0           sits a good out (declaring); the digits go into a name
;   9           turns the sound off and on (not while a name is entered)

DIRDN   EQU     1
DIRRT   EQU     2
DIRUP   EQU     4
DIRLT   EQU     8

KEYEND  EQU     13
KEYLV   EQU     27
KEYBK   EQU     8

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
INP1:   LD      A,(CTLDAT+CT_STICK)
        LD      C,A
        XOR     A
        BIT     0,C
        JR      Z,$+4
        OR      DIRUP
        BIT     1,C
        JR      Z,$+4
        OR      DIRRT
        BIT     2,C
        JR      Z,$+4
        OR      DIRDN
        BIT     3,C
        JR      Z,$+4
        OR      DIRLT
        LD      (INDIR),A
        LD      A,(CTLDAT+CT_LFIRE)
        LD      B,A
        LD      A,(CTLDAT+CT_RFIRE)
        OR      B
        AND     40H
        LD      B,A
        LD      A,(CTLDAT+CT_KEY)
        AND     0FH
        LD      HL,KPPREV
        CP      (HL)
        JR      Z,INP4          ; held: once only
        LD      (HL),A
        CALL    KEYMAP
        OR      A
        JR      Z,INP4
        LD      (LKEY),A
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

; KEYMAP: keypad code A (POLLER's) -> the game's key, or 0 (the mute key
; does its work here). Preserves B.
KEYMAP: CP      0CH
        JR      NC,KM0          ; none
        CP      0AH
        JR      Z,KMSTAR
        JR      NC,KMHASH
        CP      9
        JR      NZ,KMDIG
        LD      A,(INNAME)
        OR      A
        LD      A,9
        JR      NZ,KMDIG
        CALL    SNDMUTE
KM0:    XOR     A
        RET
KMDIG:  ADD     A,'0'
        RET
KMSTAR: LD      A,(INNAME)
        OR      A
        LD      A,KEYLV
        RET     Z
        LD      A,KEYBK
        RET
KMHASH: LD      A,KEYEND
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

; INIT: controller 1 on (POLLER reads only what is enabled).
ININIT: LD      A,0FFH
        LD      (CTLDAT+CT_EN1),A
        XOR     A
        LD      (CTLDAT+CT_EN2),A
        LD      A,0FH
        LD      (CTLDAT+CT_KEY),A
        LD      (KPPREV),A
        RET
