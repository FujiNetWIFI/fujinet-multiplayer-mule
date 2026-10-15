; ---------------------------------------------------------------------------
; input.inc -- the right hand controller and the console buttons (input.h).
;
; Read through the video's own ports: write 0 to ports 0 and 1 (port 0 bit 6
; must be clear for the controllers to answer), read, complement: a 1 is a
; press. Port 1 is the right controller: right $01, left $02, down $04,
; up $08, twist $10/$20, pull up $40, push down $80. Port 0 (low nibble)
; is the console: TIME $01, HOLD $02, MODE $04, START $08.
;
;   stick       the directions (levels)
;   push down   the button (the plunger: latched until taken)
;   START       end the turn; finish the name          (KEY_END)
;   MODE        leave the table                        (KEY_LEAVE)
;   HOLD        sit this good out, declaring           (KEY_SITOUT)
;   pull up     rub out, in name entry                 (KEY_BACK)
;   TIME        sound on/off
;
; While INPHK is non-zero the test hook stands in for the hardware:
; INPHKD the directions (DIR_*), INPHKB the button's level, INPHKK a key
; (taken once).
; ---------------------------------------------------------------------------

DIR_DOWN  EQU 1
DIR_RIGHT EQU 2
DIR_UP    EQU 4
DIR_LEFT  EQU 8

KEY_NONE  EQU 0
KEY_END   EQU 13
KEY_LEAVE EQU 27
KEY_BACK  EQU 8
KEY_SITOUT EQU '0'

; INPOLL -- every frame.  Clobbers A, r0-r3, DC0.
INPOLL: ENTER
        LDB INPHK
        NI 0FFH
        BZ INPHW
        LDB INPHKD              ; the test hook
        STB INPDIR
        LDB INPHKB
        LR 1,A
        LDB INPHKK
        LR 2,A
        CLR
        STB INPHKK
        JMP INPBT
INPHW:  CLR
        OUTS 0
        CLR
        OUTS 1
        INS 1
        COM
        LR 1,A                  ; the controller, 1 = pressed
        INS 0
        COM
        NI 00FH
        LR 0,A                  ; the console buttons
        CLR                     ; the directions, in input.h's bits
        LR 3,A
        LR A,1
        NI 001H
        BZ INPD1
        LIS DIR_RIGHT
        LR 3,A
INPD1:  LR A,1
        NI 002H
        BZ INPD2
        LR A,3
        OI DIR_LEFT
        LR 3,A
INPD2:  LR A,1
        NI 004H
        BZ INPD3
        LR A,3
        OI DIR_DOWN
        LR 3,A
INPD3:  LR A,1
        NI 008H
        BZ INPD4
        LR A,3
        OI DIR_UP
        LR 3,A
INPD4:  LR A,3
        STB INPDIR
        ; keys: what is down now and was not at the last scan
        DCI INPREV0
        LM
        COM
        NS 0
        LR 2,A                  ; new console buttons
        LR A,0
        STB INPREV0
        DCI INPREV
        LM
        COM
        NS 1
        NI 040H                 ; pull up, newly
        LR 3,A
        LR A,1
        STB INPREV
        LR A,1                  ; the button's level
        NI 080H
        LR 1,A
        LR A,3                  ; the key, by priority
        NI 0FFH
        BZ INPK1
        LIS KEY_BACK
        BR INPK5
INPK1:  LR A,2
        NI 008H
        BZ INPK2
        LIS KEY_END
        BR INPK5
INPK2:  LR A,2
        NI 004H
        BZ INPK3
        LI KEY_LEAVE
        BR INPK5
INPK3:  LR A,2
        NI 002H
        BZ INPK4
        LI KEY_SITOUT
        BR INPK5
INPK4:  LR A,2
        NI 001H
        BZ INPK6
        PI SNDTMUT              ; TIME: sound on and off
INPK6:  CLR
INPK5:  LR 2,A                  ; r2 = the key (0 none), r1 = button level
INPBT:  LR A,1                  ; the button: latched on a press
        NI 0FFH
        BZ INPB0
        LDB INPBTN
        NI 0FFH
        BNZ INPB1
        LIS 1
        STB INLBTN
INPB1:  LIS 1
        BR INPB2
INPB0:  CLR
INPB2:  STB INPBTN
        LR A,2
        NI 0FFH
        BZ INPX
        STB INLKEY
INPX:   LEAVE

; INTAKE -- the latches into EVBTN / EVKEY, and cleared.  Leaf.
;   Clobbers A, DC0.
INTAKE: LDB INLBTN
        STB EVBTN               ; (EVBTN, EVKEY follow each other)
        LDB INLKEY
        STB EVKEY
        CLR
        STB INLBTN
        ST                      ; INLKEY
        POP
