; fx.inc -- scripted sprite animations (fx.c, fx.h). The grant page; the
; summary (page 2) runs its walk-ins through the far calls at the end.
;
; The Atari plays set pieces between phases: the transport ship, the
; pirates, the pest, the acid rain cloud, the players walking into the
; status summary... Each is a little script run by one of four actors,
; stepped once a frame, so the pieces keep the Atari's speed. Script bytes
; (positions are colour clocks / Atari scanlines; SPRSET squeezes y):
;   FX_POS x+64, y+64          jump there
;   FX_MOVE dx, dy, per, n     n steps of (dx, dy) (signed), one every per frames
;   FX_WAIT n                  n frames
;   FX_LOOK kind, slot, shape, frame, d2, col, col2
;   FX_FRAME f / FX_FLAGS f / FX_HIDE / FX_COL c / FX_SIG v
;   FX_LOOP n, back            repeat the `back` bytes before this n more times
;   FX_END

FX_END  EQU     0
FX_POS  EQU     1
FX_MOVE EQU     2
FX_WAIT EQU     3
FX_LOOK EQU     4
FX_FRAME EQU    5
FX_FLAGS EQU    6
FX_HIDE EQU     7
FX_SIG  EQU     8
FX_LOOP EQU     9
FX_COL  EQU     10

FK_SINGLE EQU   1
FK_SHIP EQU     3

; an actor (FXA + FXSZ * i)
AC_ST   EQU     0
AC_XL   EQU     1
AC_XH   EQU     2
AC_YL   EQU     3
AC_YH   EQU     4
AC_WAIT EQU     5
AC_N    EQU     6
AC_PER  EQU     7
AC_DX   EQU     8
AC_DY   EQU     9
AC_KIND EQU     10
AC_SLOT EQU     11
AC_SHAPE EQU    12
AC_FRAME EQU    13
AC_D2   EQU     14
AC_COL  EQU     15
AC_COL2 EQU     16
AC_FLAGS EQU    17
AC_FBASE EQU    18
AC_ALT  EQU     19
AC_REP  EQU     20
AC_HID  EQU     21
AC_PCL  EQU     22
AC_PCH  EQU     23

; ACTOR: IX = actor A. Preserves BC, HL.
ACTOR:  PUSH    DE
        LD      E,A
        ADD     A,A             ; *24 = *3 *8
        ADD     A,E
        ADD     A,A
        ADD     A,A
        ADD     A,A
        LD      E,A
        LD      D,0
        LD      IX,FXA
        ADD     IX,DE
        POP     DE
        RET

; FXSTART: actor A runs the script at HL.
FXSTART: CALL   ACTOR
        LD      (IX+AC_ST),0
        LD      (IX+AC_PCL),L
        LD      (IX+AC_PCH),H
        LD      (IX+AC_WAIT),1
        XOR     A
        LD      (IX+AC_N),A
        LD      (IX+AC_REP),A
        LD      (IX+AC_ALT),A
        LD      (IX+AC_HID),A
        INC     A
        LD      (IX+AC_ST),A
        LD      (FXON),A
        RET

; XFXSTOP: every actor stopped, its sprites off.
XFXSTOP: LD     A,(FXON)        ; (another phase's variables may lie
        OR      A               ; over the actors: only if they ran)
        RET     Z
        XOR     A
        LD      (FXON),A
        LD      B,FXN
FXS1:   LD      A,B
        DEC     A
        CALL    ACTOR
        LD      A,(IX+AC_ST)
        OR      A
        CALL    NZ,FXHIDE
        LD      (IX+AC_ST),0
        DJNZ    FXS1
        RET

; FXHIDE: the actor's sprites off.
FXHIDE: PUSH    BC
        LD      A,(IX+AC_SLOT)
        CALL    SPRHIDE
        LD      A,(IX+AC_KIND)
        CP      FK_SHIP
        JR      NZ,FXH1
        LD      A,(IX+AC_SLOT)
        INC     A
        CALL    SPRHIDE
        LD      A,(IX+AC_SLOT)
        ADD     A,2
        CALL    SPRHIDE
FXH1:   POP     BC
        RET

; FXDRAW: the actor as it is now.
FXDRAW: LD      A,(IX+AC_HID)
        OR      A
        RET     NZ
        LD      A,(IX+AC_KIND)
        CP      FK_SHIP
        JR      NZ,FXD2
        ; a nose (8 x 10) in front of a 32 x 32 hull, light over dark
        LD      A,SH_NOSE
        LD      HL,-8
        LD      DE,10
        LD      C,(IX+AC_COL)
        LD      B,0
        CALL    FXPUT
        LD      A,SH_HTOP
        LD      HL,0
        LD      DE,0
        LD      C,(IX+AC_COL)
        LD      B,1
        CALL    FXPUT
        LD      A,SH_HBOT
        LD      HL,0
        LD      DE,16
        LD      C,(IX+AC_COL2)
        LD      B,2
        JR      FXPUT
FXD2:   ; off the edges: hidden
        LD      L,(IX+AC_XL)
        LD      H,(IX+AC_XH)
        LD      DE,32
        ADD     HL,DE
        LD      DE,168+32+1
        OR      A
        SBC     HL,DE
        JR      NC,FXDOFF       ; x < -32 or x > 168
        LD      L,(IX+AC_YL)
        LD      H,(IX+AC_YH)
        LD      DE,32
        ADD     HL,DE
        LD      DE,192+32+1
        OR      A
        SBC     HL,DE
        JR      NC,FXDOFF
        LD      A,(IX+AC_FBASE)
        ADD     A,(IX+AC_FRAME)
        LD      B,A
        LD      A,(IX+AC_ALT)
        OR      A
        LD      A,B
        JR      Z,FXD3
        ADD     A,(IX+AC_D2)
FXD3:   LD      (SPA_FR),A
        LD      A,(IX+AC_SHAPE)
        LD      (SPA_SH),A
        LD      L,(IX+AC_XL)
        LD      H,(IX+AC_XH)
        LD      (SPA_X),HL
        LD      L,(IX+AC_YL)
        LD      H,(IX+AC_YH)
        LD      (SPA_Y),HL
        LD      A,(IX+AC_COL)
        LD      (SPA_COL),A
        LD      A,(IX+AC_FLAGS)
        LD      (SPA_FL),A
        LD      A,(IX+AC_SLOT)
        JP      SPRSET
FXDOFF: LD      A,(IX+AC_SLOT)
        JP      SPRHIDE

; FXPUT: a ship part: shape A at the actor + (HL, DE), colour C, slot + B.
FXPUT:  LD      (SPA_SH),A
        LD      A,(IX+AC_FRAME)
        LD      (SPA_FR),A
        PUSH    DE
        LD      E,(IX+AC_XL)
        LD      D,(IX+AC_XH)
        ADD     HL,DE
        LD      (SPA_X),HL
        POP     DE
        LD      L,(IX+AC_YL)
        LD      H,(IX+AC_YH)
        ADD     HL,DE
        LD      (SPA_Y),HL
        LD      A,C
        LD      (SPA_COL),A
        XOR     A
        LD      (SPA_FL),A
        LD      A,(IX+AC_SLOT)
        ADD     A,B
        JP      SPRSET

; FXSTEP: one frame of actor IX (fx.c step).
FXSTEP: LD      A,(IX+AC_WAIT)
        CP      2
        JR      C,FXT1
        DEC     (IX+AC_WAIT)
        RET
FXT1:   LD      A,(IX+AC_N)
        OR      A
        JR      Z,FXT2
        LD      A,(IX+AC_DX)    ; a step of the move
        LD      E,A
        RLA
        SBC     A,A
        LD      D,A
        LD      L,(IX+AC_XL)
        LD      H,(IX+AC_XH)
        ADD     HL,DE
        LD      (IX+AC_XL),L
        LD      (IX+AC_XH),H
        LD      A,(IX+AC_DY)
        LD      E,A
        RLA
        SBC     A,A
        LD      D,A
        LD      L,(IX+AC_YL)
        LD      H,(IX+AC_YH)
        ADD     HL,DE
        LD      (IX+AC_YL),L
        LD      (IX+AC_YH),H
        DEC     (IX+AC_N)
        LD      A,(IX+AC_PER)
        LD      (IX+AC_WAIT),A
        LD      A,(IX+AC_ALT)
        XOR     1
        LD      (IX+AC_ALT),A
        JP      FXDRAW
FXT2:   LD      (IX+AC_WAIT),1  ; fetch until something takes time
        LD      L,(IX+AC_PCL)
        LD      H,(IX+AC_PCH)
FXOP:   LD      A,(HL)
        INC     HL
        OR      A
        JR      NZ,FXO1
        LD      (IX+AC_ST),0    ; FX_END
FXSAVE: LD      (IX+AC_PCL),L
        LD      (IX+AC_PCH),H
        RET
FXO1:   DEC     A
        JR      NZ,FXO2
        LD      (IX+AC_HID),0   ; FX_POS
        LD      A,(HL)
        INC     HL
        CALL    FXB64
        LD      (IX+AC_XL),E
        LD      (IX+AC_XH),D
        LD      A,(HL)
        INC     HL
        CALL    FXB64
        LD      (IX+AC_YL),E
        LD      (IX+AC_YH),D
        PUSH    HL
        CALL    FXDRAW
        POP     HL
        JR      FXOP
FXO2:   DEC     A
        JR      NZ,FXO3
        LD      A,(HL)          ; FX_MOVE
        LD      (IX+AC_DX),A
        INC     HL
        LD      A,(HL)
        LD      (IX+AC_DY),A
        INC     HL
        LD      A,(HL)
        LD      (IX+AC_PER),A
        LD      (IX+AC_WAIT),A
        INC     HL
        LD      A,(HL)
        LD      (IX+AC_N),A
        INC     HL
        JR      FXSAVE
FXO3:   DEC     A
        JR      NZ,FXO4
        LD      A,(HL)          ; FX_WAIT
        LD      (IX+AC_WAIT),A
        INC     HL
        JR      FXSAVE
FXO4:   DEC     A
        JR      NZ,FXO5
        PUSH    IX              ; FX_LOOK: kind, slot, shape, frame, d2,
        POP     DE              ; col, col2 -- 7 bytes in actor order
        LD      A,E
        ADD     A,AC_KIND
        LD      E,A
        JR      NC,FXO4A
        INC     D
FXO4A:  LD      BC,7
        LDIR
        XOR     A
        LD      (IX+AC_FLAGS),A
        LD      (IX+AC_FBASE),A
        LD      (IX+AC_HID),A
        JP      FXOP
FXO5:   DEC     A
        JR      NZ,FXO6
        LD      (IX+AC_HID),0   ; FX_FRAME
        LD      A,(HL)
        INC     HL
        LD      (IX+AC_FRAME),A
FXDRW:  PUSH    HL
        CALL    FXDRAW
        POP     HL
        JP      FXOP
FXO6:   DEC     A
        JR      NZ,FXO7
        LD      A,(HL)          ; FX_FLAGS
        INC     HL
        LD      (IX+AC_FLAGS),A
        JR      FXDRW
FXO7:   DEC     A
        JR      NZ,FXO8
        PUSH    HL              ; FX_HIDE
        CALL    FXHIDE
        POP     HL
        LD      (IX+AC_HID),1
        JP      FXOP
FXO8:   DEC     A
        JR      NZ,FXO9
        LD      A,(HL)          ; FX_SIG
        INC     HL
        LD      (FXFLAG),A
        JP      FXOP
FXO9:   DEC     A
        JR      NZ,FXO10
        LD      A,(IX+AC_REP)   ; FX_LOOP n, back
        OR      A
        JR      NZ,FXO9A
        LD      A,(HL)
        INC     A
FXO9A:  DEC     A
        LD      (IX+AC_REP),A
        JR      Z,FXO9B
        INC     HL              ; pc = pc - 1 - back (pc at n)
        LD      E,(HL)
        DEC     HL
        DEC     HL
        LD      D,0
        OR      A
        SBC     HL,DE
        JP      FXOP
FXO9B:  INC     HL
        INC     HL
        JP      FXOP
FXO10:  DEC     A
        JR      NZ,FXOBAD
        LD      A,(HL)          ; FX_COL
        INC     HL
        LD      (IX+AC_COL),A
        JR      FXDRW
FXOBAD: LD      (IX+AC_ST),0
        RET

; FXB64: DE = the position byte A - 64 (signed).
FXB64:  SUB     64
        LD      E,A
        LD      D,0
        RET     NC
        DEC     D
        RET

; FXRUN: every frame: each running actor a step (fx.c fx_run).
FXRUN:  LD      A,(FXON)
        OR      A
        RET     Z
        XOR     A
FXR1:   PUSH    AF
        CALL    ACTOR
        LD      A,(IX+AC_ST)
        OR      A
        CALL    NZ,FXSTEP
        POP     AF
        INC     A
        CP      FXN
        JR      C,FXR1
        RET

; ---- the transport ship (fx.c), for the summary, the grant, production ----
SHIPY   EQU     74              ; hull top (Atari line 103)
DOCKX   EQU     76
DOCKY   EQU     82
XB      MACRO   pos
        DB      (pos)+64
        ENDM

; the ship lands (round 1's arrival, "the ship is back")
FXLAND: DB      FX_LOOK,FK_SHIP,5,SH_NOSE,0,0,CW,CB
        DB      FX_POS
        XB      182
        XB      SHIPY
        DB      FX_WAIT,36
        DB      FX_MOVE,-1 AND 0FFH,0,1,230     ; a fast pass right to left
        DB      FX_HIDE,FX_WAIT,18
        DB      FX_POS
        XB      -40
        XB      SHIPY
        DB      FX_MOVE,1,0,6,78                ; a slow return
        DB      FX_MOVE,5,0,6,9                 ; settling onto the store
        DB      FX_HIDE
        DB      FX_LOOK,FK_SINGLE,8,SH_DOCK,0,0,CB,0    ; sinking into it
        DB      FX_POS
        XB      DOCKX
        XB      DOCKY
        DB      FX_WAIT,8
        DB      FX_FRAME,1,FX_WAIT,8,FX_FRAME,2,FX_WAIT,8
        DB      FX_FRAME,3,FX_WAIT,8,FX_HIDE
        DB      FX_SIG,1
        DB      FX_END

; the ship leaves (after round 1's summary: "back in 12 months")
FXDEPART: DB    FX_WAIT,45
        DB      FX_LOOK,FK_SINGLE,8,SH_DOCK,3,0,CB,0    ; rising out of the store
        DB      FX_POS
        XB      DOCKX
        XB      DOCKY
        DB      FX_WAIT,8
        DB      FX_FRAME,2,FX_WAIT,8,FX_FRAME,1,FX_WAIT,8
        DB      FX_FRAME,0,FX_WAIT,8,FX_HIDE
        DB      FX_LOOK,FK_SHIP,5,SH_NOSE,0,0,CW,CB
        DB      FX_POS
        XB      85
        XB      SHIPY
        DB      FX_MOVE,-5 AND 0FFH,0,8,9
        DB      FX_MOVE,-1 AND 0FFH,0,8,90      ; off to the left
        DB      FX_HIDE
        DB      FX_SIG,1
        DB      FX_END
