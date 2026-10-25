; fx.inc -- scripted sprite animations (mekkogx fx.c): the transport ship,
; the pirates, the colony events' actors, the players walking into the
; summary. Four actors each run a little script, stepped once a frame (more
; to catch up), so the set pieces keep the Atari's speed. Positions are the
; Studio II's own: x 0-63 pixels, y 0-119 lines, signed bytes.
;
; Script bytes:
;   FX_POS x, y            jump there
;   FX_MOVE dx, dy, per, n n steps of (dx, dy), one every per frames
;   FX_WAIT n              n frames
;   FX_LOOK kind, slot, img, frame, d2
;                          what it looks like: FK_SINGLE (one sprite, picture
;                          img + frame, + d2 on every other step: walking)
;                          or FK_SHIP (img and img + 1 side by side)
;   FX_FRAME f             change the frame
;   FX_MODE m              the sprite's mode (TM_*)
;   FX_HIDE                off the screen (until the next POS, FRAME, LOOK)
;   FX_SIG v               FXFLAG = v, for the game to see
;   FX_LOOP n, back        repeat the `back` bytes before this n more times
;   FX_END                 stop (the sprites stay as they are)

FX_END   EQU    0
FX_POS   EQU    1
FX_MOVE  EQU    2
FX_WAIT  EQU    3
FX_LOOK  EQU    4
FX_FRAME EQU    5
FX_MODE  EQU    6
FX_HIDE  EQU    7
FX_SIG   EQU    8
FX_LOOP  EQU    9
FK_SINGLE EQU   1
FK_SHIP  EQU    2
FS_RUN  EQU     1               ; FXST bits
FS_HID  EQU     2
FS_ALT  EQU     4

; An actor's field: FGET arr (D = it), FPUT arr (it = D); RC.0 the actor.
FGET    MACRO   arr
        GLO     RC
        ADI     LO(arr)
        PLO     RA
        LDN     RA
        ENDM
FPUT    MACRO   arr
        PHI     RD
        GLO     RC
        ADI     LO(arr)
        PLO     RA
        GHI     RD
        STR     RA
        ENDM

; fx_start: actor D runs the script at R9.
        RFIT    67
fx_start:
        PLO     RC
        GLO     R9
        FPUT    FXPCL
        GHI     R9
        FPUT    FXPCH
        LDI     1
        FPUT    FXWAIT
        LDI     0
        FPUT    FXN
        FPUT    FXREP
        LDI     TM_OR
        FPUT    FXMODE
        LDI     FS_RUN
        FPUT    FXST
        SETV    FXON, 1
        FRET

; fx_stop_all: every actor stopped, its sprites off.
        RFIT    37
fx_stop_all:
        SETV    FXON, 0
        LDI     0
        PLO     RC
fsa1:   FGET    FXST
        ANI     FS_RUN
        BZ      fsa2
        FCALL   fx_hide
fsa2:   LDI     0
        FPUT    FXST
        INC     RC
        GLO     RC
        XRI     4
        BNZ     fsa1
        FRET

; fx_run: every frame (main.c's catch-up): each running actor a step.
        RFIT    43
fx_run: GETV    FXON
        BZ      fxr9
        LDI     0
        PUTV    FXI
fxr1:   GETV    FXI
        PLO     RC
        FGET    FXST
        ANI     FS_RUN
        BZ      fxr2
        FCALL   fx_step
fxr2:   GETV    FXI
        ADI     1
        STR     RA
        XRI     4
        BNZ     fxr1
fxr9:   FRET

; fx_hide: actor RC's sprites off (moved off the right edge).
        RFIT    50
fx_hide:
        FGET    FXSLOT
        FCALL   fh1
        FGET    FXKIND
        XRI     FK_SHIP
        BNZ     fh9
        FGET    FXSLOT
        ADI     1
fh1:    STR     R2
        PARS
        LDN     R2
        PD
        P       100
        P       0
        GOP     TOP_SPRMOVE
fh9:    FRET

; fx_draw: actor RC as it looks now (unless hidden).
        RFIT    161
fx_draw:
        FGET    FXST
        ANI     FS_HID
        BNZ     fd9
        FGET    FXKIND
        XRI     FK_SHIP
        BZ      fd2
        FGET    FXIMG                   ; one: img + frame (+ d2 every other step)
        STR     R2
        FGET    FXFRAME
        ADD
        STR     R2
        FGET    FXST
        ANI     FS_ALT
        BZ      fd1
        FGET    FXD2
        ADD
        STR     R2
fd1:    PARS
        FGET    FXSLOT
        PD
        LDN     R2
        PD
        FGET    FXX
        PD
        FGET    FXY
        PD
        FGET    FXMODE
        PD
        GOP     TOP_SPRITE
fd9:    FRET
fd2:    LDI     0                       ; a ship: two halves
        FCALL   fd3
        LDI     1
fd3:    PLO     R9
        PARS
        FGET    FXSLOT
        STR     R2
        GLO     R9
        ADD
        PD
        FGET    FXIMG
        STR     R2
        GLO     R9
        ADD
        PD
        GLO     R9
        SHL
        SHL
        SHL
        STR     R2
        FGET    FXX
        ADD
        PD
        FGET    FXY
        PD
        FGET    FXMODE
        PD
        GOP     TOP_SPRITE
        FRET

; fx_step: actor RC's frame (mekkogx step).
        RFIT    96
fx_step:
        FGET    FXWAIT                  ; waiting
        SMI     2
        BNF     fs1
        ADI     1
        STR     RA
        FRET
fs1:    FGET    FXN                     ; moving: a step
        BZ      fs2
        SMI     1
        STR     RA
        FGET    FXDX
        STR     R2
        FGET    FXX
        ADD
        STR     RA
        FGET    FXDY
        STR     R2
        FGET    FXY
        ADD
        STR     RA
        FGET    FXPER
        FPUT    FXWAIT
        FGET    FXST
        XRI     FS_ALT
        STR     RA
        FJMP    fx_draw
fs2:    LDI     1                       ; fetch until something takes time
        FPUT    FXWAIT
        FGET    FXPCH
        PHI     R9
        FGET    FXPCL
        PLO     R9
        FJMP    fx_fetch

; fx_fetch: the script at R9 for actor RC, until an op takes time.
        RFIT    185
fx_fetch:
        LDA     R9
        BZ      ff_end
        SMI     1
        BZ      ff_pos
        SMI     1
        BZ      ff_move
        SMI     1
        BZ      ff_wait
        SMI     1
        BZ      ff_look2
        SMI     1
        BZ      ff_frame2
        SMI     1
        BZ      ff_mode2
        SMI     1
        BZ      ff_hide2
        SMI     1
        BZ      ff_sig2
        SMI     1
        BZ      ff_loop2
ff_end: LDI     0                       ; (an unknown op ends it too)
        FPUT    FXST
        FRET
ff_pos: FGET    FXST                    ; shown again
        ANI     FS_RUN|FS_ALT
        STR     RA
        LDA     R9
        FPUT    FXX
        LDA     R9
        FPUT    FXY
        FCALL   ff_save
        FCALL   fx_draw
        FJMP    fx_fetch2
ff_move:
        LDA     R9
        FPUT    FXDX
        LDA     R9
        FPUT    FXDY
        LDA     R9
        FPUT    FXPER
        FPUT    FXWAIT
        LDA     R9
        FPUT    FXN
        FJMP    ff_save
ff_wait:
        LDA     R9
        FPUT    FXWAIT
        FJMP    ff_save
ff_look2:
        FJMP    ff_look
ff_frame2:
        FJMP    ff_frame
ff_mode2:
        FJMP    ff_mode
ff_hide2:
        FJMP    ff_hide
ff_sig2:
        FJMP    ff_sig
ff_loop2:
        FJMP    ff_loop

; ff_save: the actor's script position = R9.
ff_save:
        GLO     R9
        FPUT    FXPCL
        GHI     R9
        FPUT    FXPCH
        FRET
; fx_fetch2: on with the script from the saved position (after a call).
fx_fetch2:
        FGET    FXPCH
        PHI     R9
        FGET    FXPCL
        PLO     R9
        FJMP    fx_fetch

        RFIT    148
ff_look:
        LDA     R9
        FPUT    FXKIND
        LDA     R9
        FPUT    FXSLOT
        LDA     R9
        FPUT    FXIMG
        LDA     R9
        FPUT    FXFRAME
        LDA     R9
        FPUT    FXD2
        FGET    FXST                    ; shown, the first walk frame
        ANI     FS_RUN
        STR     RA
        FJMP    fx_fetch
ff_frame:
        FGET    FXST
        ANI     FS_RUN|FS_ALT
        STR     RA
        LDA     R9
        FPUT    FXFRAME
        BR      ffd
ff_mode:
        LDA     R9
        FPUT    FXMODE
ffd:    FCALL   ff_save
        FCALL   fx_draw
        FJMP    fx_fetch2
ff_hide:
        FCALL   ff_save
        FCALL   fx_hide
        FGET    FXST
        ORI     FS_HID
        STR     RA
        FJMP    fx_fetch2
ff_sig: LDA     R9
        PUTV    FXFLAG
        FJMP    fx_fetch
ff_loop:
        FGET    FXREP                   ; c = rep, or n + 1 the first time
        BNZ     fl1
        LDN     R9
        ADI     1
fl1:    SMI     1                       ; rep = --c
        STR     RA
        BZ      fl2
        INC     R9                      ; again: `back` bytes before the op
        LDN     R9                      ; (R9 on back: the op is 2 before)
        ADI     2
        STR     R2
        GLO     R9
        SM
        PLO     R9
        GHI     R9
        SMBI    0
        PHI     R9
        FJMP    fx_fetch
fl2:    INC     R9                      ; done: on past n, back
        INC     R9
        FJMP    fx_fetch

; ---- the transport ship (mekkogx fx.c), on the map: the store is plot 22 ----
        DFIT    100
SHIPS_SEGEND:                           ; (data from here: no page rules)
SHIPY   EQU     MAPY0+31                ; the hull's top, over the store
DOCKX   EQU     28                      ; the store's plot
DOCKY   EQU     MAPY0+38

; landing: round 0's arrival, production's "the ship is back"
fx_land:
        DB      FX_LOOK, FK_SHIP, 5, IM_SHIPL, 0, 0
        DB      FX_POS, 70, SHIPY
        DB      FX_WAIT, 36
        DB      FX_MOVE, -2, 0, 5, 46           ; a fast pass, right to left
        DB      FX_HIDE, FX_WAIT, 18
        DB      FX_POS, -16, SHIPY
        DB      FX_MOVE, 1, 0, 15, 30           ; the slow return
        DB      FX_MOVE, 2, 0, 6, 5             ; settling over the store
        DB      FX_HIDE
        DB      FX_LOOK, FK_SINGLE, 7, IM_DOCK0, 0, 0       ; sinking into it
        DB      FX_POS, DOCKX-1, DOCKY, FX_WAIT, 8
        DB      FX_FRAME, 1, FX_WAIT, 8, FX_FRAME, 2, FX_WAIT, 8
        DB      FX_FRAME, 3, FX_WAIT, 8, FX_HIDE
        DB      FX_SIG, 1
        DB      FX_END

; leaving: the grant of round 1 ("the ship will be back in 12 months")
fx_depart:
        DB      FX_WAIT, 45
        DB      FX_LOOK, FK_SINGLE, 7, IM_DOCK0, 3, 0       ; rising out of the store
        DB      FX_POS, DOCKX-1, DOCKY, FX_WAIT, 8
        DB      FX_FRAME, 2, FX_WAIT, 8, FX_FRAME, 1, FX_WAIT, 8
        DB      FX_FRAME, 0, FX_WAIT, 8, FX_HIDE
        DB      FX_LOOK, FK_SHIP, 5, IM_SHIPL, 0, 0
        DB      FX_POS, 24, SHIPY
        DB      FX_MOVE, -2, 0, 8, 4
        DB      FX_MOVE, -1, 0, 24, 32          ; off to the left, slowly
        DB      FX_HIDE
        DB      FX_SIG, 1
        DB      FX_END
