; ---------------------------------------------------------------------------
; fx.inc -- scripted sprite animations (mekkogx fx.c, fx.h): four actors,
; each running a little script a step a frame, so the set pieces keep the
; Atari's speed. The scripts are in the data page (fxdata.inc), or built in
; RAM (develop's computer walks).
;
; Script bytes (positions are colour clocks / scanlines):
;   FX_POS x+64, y+64          jump there
;   FX_MOVE dx, dy, per, n     n steps of (dx, dy) (signed), one every per
;   FX_WAIT n                  n frames
;   FX_LOOK kind, slot, shape, frame, d2, col, col2
;   FX_FRAME f / FX_FLAGS f / FX_HIDE / FX_COL c / FX_SIG v
;   FX_LOOP n, back            repeat the `back` bytes before this n more times
;   FX_END
;
; An actor steps in FXW (a copy at a fixed place, so its fields are plain
; addresses), then goes back. The script pointer rides in DC1, and is put
; away round anything that draws (SPRSET uses both DCs).
; ---------------------------------------------------------------------------

FX_END  EQU 0
FX_POS  EQU 1
FX_MOVE EQU 2
FX_WAIT EQU 3
FX_LOOK EQU 4
FX_FRAME EQU 5
FX_FLAGS EQU 6
FX_HIDE EQU 7
FX_SIG  EQU 8
FX_LOOP EQU 9
FX_COL  EQU 10
FK_SINGLE EQU 1
FK_SHIP EQU 3

FXSZ    EQU 24                  ; an actor (fx.h Actor)
AC_ST    EQU 0                   ; running
AC_XL    EQU 1                   ; x, y: signed 16
AC_XH    EQU 2
AC_YL    EQU 3
AC_YH    EQU 4
AC_WAIT  EQU 5
AC_N     EQU 6
AC_PER   EQU 7
AC_DX    EQU 8                   ; signed
AC_DY    EQU 9
AC_KIND  EQU 10
AC_SLOT  EQU 11
AC_SHAPE EQU 12
AC_FRAME EQU 13
AC_D2    EQU 14
AC_COL   EQU 15
AC_COL2  EQU 16
AC_FLAGS EQU 17
AC_FBASE EQU 18
AC_ALT   EQU 19
AC_REP   EQU 20
AC_HIDN  EQU 21
AC_PCH   EQU 22                  ; the script (big-endian)
AC_PCL   EQU 23

FXW     EQU FXA+4*FXSZ          ; the actor stepping
FXI     EQU FXA+5*FXSZ          ; which one it is

; PCSAVE / PCLOAD -- the script pointer (DC1) to FXW and back.  Clobber A, H.
PCSAVE  MACRO
        XDC
        LR H,DC
        XDC
        STW 10, 11, FXW+AC_PCH
        ENDM
PCLOAD  MACRO
        LDW 10, 11, FXW+AC_PCH
        LR DC,H
        XDC
        ENDM

; FXNEXT -- A = the script's next byte.  Clobbers nothing else.
FXNEXT  MACRO
        XDC
        LM
        XDC
        ENDM

; FXAT -- DC0 = actor r0's record.  Leaf.  Clobbers A, r8.
FXAT:   LR A,0
        SL 1
        SL 1
        SL 1
        LR 8,A
        SL 1
        AS 8                    ; 24 * actor
        DCI FXA
        ADC
        POP

; FXLOAD / FXSAVE -- actor FXI into FXW, and back.  Clobber A, r0, r1, r8,
;   DC0, DC1.
FXLOAD: ENTER
        LDB FXI
        LR 0,A
        PI FXAT
        XDC
        DCI FXW
        XDC
        LI FXSZ
        LR 0,A
        CLR
        LR 1,A
        PI MEMCPY
        LEAVE

FXSAVE: ENTER
        LDB FXI
        LR 0,A
        PI FXAT
        XDC
        DCI FXW
        LI FXSZ
        LR 0,A
        CLR
        LR 1,A
        PI MEMCPY
        LEAVE

; ---------------------------------------------------------------------------
; FXSTART -- actor r0 runs the script at H (fx.c fx_start). Its look (kind,
;   slot ...) is whatever the caller set, or the script's LOOK.
;   Clobbers A, r8, DC0.
; ---------------------------------------------------------------------------
FXSTART: ENTER
        PI FXAT
        CLR
        ST                      ; not running while it is set up
        LI AC_WAIT-1
        ADC
        LIS 1
        ST                      ; wait 1
        CLR
        ST                      ; n 0
        PI FXAT
        LI AC_ALT
        ADC
        CLR
        ST                      ; alt, rep, hidden
        ST
        ST
        LR A,10
        ST                      ; the script
        LR A,11
        ST
        PI FXAT
        LIS 1
        ST                      ; running
        STB FXON
        LEAVE

; FXSTOP -- every actor stopped, its sprites off (fx.c fx_stop_all).
FXSTOP: ENTER
        CLR
        STB FXON
        CLR
        STB FXI
FXST1:  LDB FXI
        LR 0,A
        PI FXAT
        LM
        NI 0FFH
        BZ FXST2
        PI FXLOAD
        PI FXHIDE
        LDB FXI
        LR 0,A
        PI FXAT
        CLR
        ST
FXST2:  INCB FXI
        CI 3
        BC FXST1
        LEAVE

; FXRUN -- every frame: each running actor a step (fx.c fx_run).
FXRUN:  ENTER
        LDB FXON
        NI 0FFH
        BZ FXRX
        CLR
        STB FXI
FXR1:   LDB FXI
        LR 0,A
        PI FXAT
        LM
        NI 0FFH
        BZ FXR2
        PI FXLOAD
        PI FXSTEP
        PI FXSAVE
FXR2:   INCB FXI
        CI 3
        BC FXR1
FXRX:   LEAVE

; ---------------------------------------------------------------------------
; FXHIDE -- the actor's sprites off (a ship has three).
; ---------------------------------------------------------------------------
FXHIDE: ENTER
        LDB FXW+AC_SLOT
        LR 0,A
        PI SPRHIDE
        LDB FXW+AC_KIND
        CI FK_SHIP
        BNZ FXH1
        LDB FXW+AC_SLOT
        INC
        LR 0,A
        PI SPRHIDE
        LDB FXW+AC_SLOT
        AI 2
        LR 0,A
        PI SPRHIDE
FXH1:   LEAVE

; ---------------------------------------------------------------------------
; FXDRAW -- the actor in FXW onto its sprites (fx.c draw). A ship is a nose
;   in front of a hull, light over dark; anything else is one sprite, hidden
;   when it is off every screen.
; ---------------------------------------------------------------------------
FXDRAW: ENTER
        LDB FXW+AC_HIDN
        NI 0FFH
        JNZ FXDX
        LDB FXW+AC_KIND
        CI FK_SHIP
        JNZ FXD1
        LDB FXW+AC_SLOT          ; the nose: (x - 8, y + 10)
        STB SPA_SL
        LI SH_NOSE
        STB SPA_SH
        LDB FXW+AC_FRAME
        STB SPA_FR
        LDW 0, 1, FXW+AC_XL
        LIS 8
        LR 2,A
        CLR
        LR 3,A
        PI SUB16
        STW 0, 1, SPA_XL
        LDW 0, 1, FXW+AC_YL
        LI 10
        LR 2,A
        CLR
        LR 3,A
        PI ADD16
        STW 0, 1, SPA_YL
        LDB FXW+AC_COL
        STB SPA_CO
        CLR
        STB SPA_FL
        PI SPRSET
        LDB FXW+AC_SLOT          ; the hull's top: (x, y)
        INC
        STB SPA_SL
        LI SH_HULLT
        STB SPA_SH
        LDW 0, 1, FXW+AC_XL
        STW 0, 1, SPA_XL
        LDW 0, 1, FXW+AC_YL
        STW 0, 1, SPA_YL
        PI SPRSET
        LDB FXW+AC_SLOT          ; its bottom: (x, y + 16), the dark colour
        AI 2
        STB SPA_SL
        LI SH_HULLB
        STB SPA_SH
        LDW 0, 1, FXW+AC_YL
        LI 16
        LR 2,A
        CLR
        LR 3,A
        PI ADD16
        STW 0, 1, SPA_YL
        LDB FXW+AC_COL2
        STB SPA_CO
        PI SPRSET
        LEAVE
FXD1:   LDW 0, 1, FXW+AC_XL      ; -32 <= x <= 168?
        LI 32
        LR 2,A
        CLR
        LR 3,A
        PI ADD16
        LR A,1
        NI 0FFH
        JNZ FXDOFF
        LR A,0
        CI 200
        JNC FXDOFF
        LDW 0, 1, FXW+AC_YL      ; -32 <= y <= 192?
        LI 32
        LR 2,A
        CLR
        LR 3,A
        PI ADD16
        LR A,1
        NI 0FFH
        JNZ FXDOFF
        LR A,0
        CI 224
        JNC FXDOFF
        LDB FXW+AC_SLOT
        STB SPA_SL
        LDB FXW+AC_SHAPE
        STB SPA_SH
        LDB FXW+AC_ALT           ; fbase + frame (+ d2 on the other step)
        NI 0FFH
        LDB FXW+AC_D2
        BNZ FXD2
        CLR
FXD2:   LR 4,A
        LDB FXW+AC_FRAME
        AS 4
        LR 4,A
        LDB FXW+AC_FBASE
        AS 4
        STB SPA_FR
        LDW 0, 1, FXW+AC_XL
        STW 0, 1, SPA_XL
        LDW 0, 1, FXW+AC_YL
        STW 0, 1, SPA_YL
        LDB FXW+AC_COL
        STB SPA_CO
        LDB FXW+AC_FLAGS
        STB SPA_FL
        PI SPRSET
FXDX:   LEAVE
FXDOFF: LDB FXW+AC_SLOT
        LR 0,A
        PI SPRHIDE
        LEAVE

; ---------------------------------------------------------------------------
; FXSTEP -- one frame of the actor in FXW (fx.c step).
; ---------------------------------------------------------------------------
FXSTEP: ENTER
        LDB FXW+AC_WAIT          ; waiting
        CI 1
        BC FXS1
        AI 0FFH
        STB FXW+AC_WAIT
        LEAVE
FXS1:   LDB FXW+AC_N             ; moving: a step
        NI 0FFH
        JZ FXS2
        AI 0FFH
        STB FXW+AC_N
        LDB FXW+AC_DX            ; x += dx (signed)
        LR 2,A
        NI 080H
        LIS 0
        BZ FXS1A
        LI 0FFH
FXS1A:  LR 3,A
        LDW 0, 1, FXW+AC_XL
        PI ADD16
        STW 0, 1, FXW+AC_XL
        LDB FXW+AC_DY            ; y += dy
        LR 2,A
        NI 080H
        LIS 0
        BZ FXS1B
        LI 0FFH
FXS1B:  LR 3,A
        LDW 0, 1, FXW+AC_YL
        PI ADD16
        STW 0, 1, FXW+AC_YL
        LDB FXW+AC_PER
        STB FXW+AC_WAIT
        LDB FXW+AC_ALT
        XI 1
        STB FXW+AC_ALT
        PI FXDRAW
        LEAVE
FXS2:   LIS 1                   ; the script, until something takes time
        STB FXW+AC_WAIT
        PCLOAD
FXOP:   FXNEXT
        NI 0FFH
        JZ FXEND
        CI FX_POS
        JZ FXPOS
        CI FX_MOVE
        JZ FXMOVE
        CI FX_WAIT
        JZ FXWAIT
        CI FX_LOOK
        JZ FXLOOK
        CI FX_FRAME
        JZ FXFRM
        CI FX_FLAGS
        JZ FXFLG
        CI FX_HIDE
        JZ FXHID
        CI FX_SIG
        JZ FXSIG
        CI FX_LOOP
        JZ FXLOOP
        CI FX_COL
        JZ FXCOL
FXEND:  CLR                     ; (an unknown op ends it too)
        STB FXW+AC_ST
        PCSAVE
        LEAVE
FXPOS:  CLR
        STB FXW+AC_HIDN
        FXNEXT                  ; x = byte - 64
        AI 0C0H                 ; (carries when the byte was 64 or more)
        LR 4,A
        LIS 0
        BC FXP1
        LI 0FFH
FXP1:   LR 5,A
        FXNEXT
        AI 0C0H
        LR 6,A
        LIS 0
        BC FXP2
        LI 0FFH
FXP2:   LR 7,A
        STW 4, 5, FXW+AC_XL
        STW 6, 7, FXW+AC_YL
        PCSAVE
        PI FXDRAW
        PCLOAD
        JMP FXOP
FXMOVE: DCI FXW+AC_DX            ; dx, dy, per, n
        FXNEXT
        ST
        FXNEXT
        ST
        DCI FXW+AC_PER
        FXNEXT
        ST
        STB FXW+AC_WAIT
        FXNEXT
        STB FXW+AC_N
        PCSAVE
        LEAVE
FXWAIT: FXNEXT
        STB FXW+AC_WAIT
        PCSAVE
        LEAVE
FXLOOK: DCI FXW+AC_KIND          ; kind, slot, shape, frame, d2, col, col2
        LIS 7
        LR 4,A
FXLK1:  FXNEXT
        ST
        DS 4
        BNZ FXLK1
        CLR
        ST                      ; flags
        ST                      ; fbase
        STB FXW+AC_HIDN
        JMP FXOP
FXFRM:  CLR
        STB FXW+AC_HIDN
        FXNEXT
        STB FXW+AC_FRAME
        BR FXDRW
FXFLG:  FXNEXT
        STB FXW+AC_FLAGS
        BR FXDRW
FXCOL:  FXNEXT
        STB FXW+AC_COL
FXDRW:  PCSAVE
        PI FXDRAW
        PCLOAD
        JMP FXOP
FXHID:  PCSAVE
        PI FXHIDE
        PCLOAD
        LIS 1
        STB FXW+AC_HIDN
        JMP FXOP
FXSIG:  FXNEXT
        STB FXFLAG
        JMP FXOP
FXLOOP: FXNEXT                  ; n, back
        LR 4,A
        FXNEXT
        LR 5,A
        LDB FXW+AC_REP           ; c = rep, or n + 1 the first time
        NI 0FFH
        BNZ FXL1
        LR A,4
        INC
FXL1:   AI 0FFH                 ; rep = --c
        STB FXW+AC_REP
        NI 0FFH
        JZ FXOP                 ; done: on past it
        XDC                     ; again: back to (the op) - back
        LR A,5
        AI 3
        COM
        INC
        ADC
        XDC
        JMP FXOP
