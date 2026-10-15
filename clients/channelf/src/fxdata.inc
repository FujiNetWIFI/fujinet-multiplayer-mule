; ---------------------------------------------------------------------------
; fxdata.inc -- the set pieces' scripts (mekkogx fx.c, events.c, status.c),
; byte for byte. In the data page: they are read from RAM whatever page is
; mapped. XB(x) is a position byte (x + 64), SB(v) a signed one.
; ---------------------------------------------------------------------------

SHIP_Y  EQU 74                  ; hull top (Atari line 103)
DOCK_X  EQU 76
DOCK_Y  EQU 82
PIRATE_Y EQU 66                 ; the pirate's hull sits 8 lines lower

; Transport ship landing (round 1 arrival, round 12 "the ship is back").
FXLAND: DB FX_LOOK, FK_SHIP, 5, SH_NOSE, 0, 0, C_WHITE, C_BLUE
        DB FX_POS, XB(182), XB(SHIP_Y)
        DB FX_WAIT, 36
        DB FX_MOVE, SB(-1), 0, 1, 230   ; fast pass right to left, 1 cc a frame
        DB FX_HIDE, FX_WAIT, 18
        DB FX_POS, XB(-40), XB(SHIP_Y)
        DB FX_MOVE, 1, 0, 6, 78         ; slow return, 1 cc every 6 frames
        DB FX_MOVE, 5, 0, 6, 9          ; settling onto the store
        DB FX_HIDE
        DB FX_LOOK, FK_SINGLE, 8, SH_DOCK, 0, 0, C_LTBLUE, 0   ; sinking into it
        DB FX_POS, XB(DOCK_X), XB(DOCK_Y), FX_WAIT, 8
        DB FX_FRAME, 1, FX_WAIT, 8, FX_FRAME, 2, FX_WAIT, 8
        DB FX_FRAME, 3, FX_WAIT, 8, FX_HIDE
        DB FX_SIG, 1
        DB FX_END

; Transport ship leaving (after round 1's summary: "back in 12 months").
FXDEPART: DB FX_WAIT, 45
        DB FX_LOOK, FK_SINGLE, 8, SH_DOCK, 3, 0, C_LTBLUE, 0   ; rising out of the store
        DB FX_POS, XB(DOCK_X), XB(DOCK_Y), FX_WAIT, 8
        DB FX_FRAME, 2, FX_WAIT, 8, FX_FRAME, 1, FX_WAIT, 8
        DB FX_FRAME, 0, FX_WAIT, 8, FX_HIDE
        DB FX_LOOK, FK_SHIP, 5, SH_NOSE, 0, 0, C_WHITE, C_BLUE
        DB FX_POS, XB(85), XB(SHIP_Y)
        DB FX_MOVE, SB(-5), 0, 8, 9
        DB FX_MOVE, SB(-1), 0, 8, 90    ; off to the left, 1 cc every 8 frames
        DB FX_HIDE
        DB FX_SIG, 1
        DB FX_END

; Pirate ship: fast pass, back to the store, raid, away.
FXPIRATE: DB FX_LOOK, FK_SHIP, 5, SH_NOSE, 1, 0, C_WHITE, C_RED
        DB FX_POS, XB(182), XB(PIRATE_Y)
        DB FX_WAIT, 36
        DB FX_MOVE, SB(-1), 0, 1, 230
        DB FX_HIDE, FX_WAIT, 18
        DB FX_POS, XB(-40), XB(PIRATE_Y)
        DB FX_MOVE, 1, 0, 4, 78
        DB FX_MOVE, 5, 0, 6, 9
        DB FX_SIG, 2                    ; landed: "pirates take all crystite!"
        DB FX_WAIT, 150
        DB FX_MOVE, SB(-5), 0, 8, 9
        DB FX_MOVE, SB(-1), 0, 4, 90
        DB FX_HIDE
        DB FX_SIG, 1
        DB FX_END

; Pest: crawls around its plot (x/y set first), 2 cc every 5 frames,
; pausing 11 frames at each turn.
FXPEST: DB FX_LOOK, FK_SINGLE, 3, SH_PEST, 0, 1, C_WHITE, 0
        DB FX_FRAME, 0
        DB FX_MOVE, 2, 0, 5, 3, FX_WAIT, 11
        DB FX_MOVE, 0, 2, 5, 3, FX_WAIT, 11
        DB FX_MOVE, SB(-2), 0, 5, 3, FX_WAIT, 11
        DB FX_MOVE, 0, SB(-2), 5, 3, FX_WAIT, 11
        DB FX_LOOP, 6, 28
        DB FX_HIDE
        DB FX_SIG, 1
        DB FX_END

; Acid rain: the cloud drifts along its row, its colour cycling, with its
; lightning 24 lines under it, flickering.
FXCLOUD: DB FX_LOOK, FK_SINGLE, 3, SH_CLOUD, 0, 0, C_WHITE, 0
        DB FX_FRAME, 0, FX_COL, C_WHITE, FX_MOVE, 1, 0, 4, 5
        DB FX_FRAME, 1, FX_COL, C_BLUE, FX_MOVE, 1, 0, 4, 5
        DB FX_FRAME, 2, FX_COL, C_WHITE, FX_MOVE, 1, 0, 4, 5
        DB FX_FRAME, 3, FX_COL, C_LTBLUE, FX_MOVE, 1, 0, 4, 5
        DB FX_LOOP, 7, 36
        DB FX_HIDE
        DB FX_SIG, 1
        DB FX_END
FXBOLT: DB FX_LOOK, FK_SINGLE, 4, SH_BOLT, 0, 0, C_YELLOW, 0
        DB FX_FRAME, 0, FX_MOVE, 1, 0, 4, 5
        DB FX_HIDE, FX_MOVE, 1, 0, 4, 5
        DB FX_FRAME, 0, FX_MOVE, 1, 0, 4, 5
        DB FX_HIDE, FX_MOVE, 1, 0, 4, 5
        DB FX_LOOP, 7, 26
        DB FX_HIDE
        DB FX_END

; Sunspot sparks: single pixels flickering around the map.
FXSPARKS: DB FX_LOOK, FK_SINGLE, 3, SH_SPARK, 0, 0, C_WHITE, 0
        DB FX_POS, XB(32), XB(44), FX_WAIT, 6, FX_POS, XB(112), XB(124), FX_WAIT, 6
        DB FX_POS, XB(64), XB(28), FX_WAIT, 6, FX_POS, XB(142), XB(84), FX_WAIT, 6
        DB FX_POS, XB(12), XB(144), FX_WAIT, 6, FX_POS, XB(92), XB(50), FX_WAIT, 6
        DB FX_POS, XB(52), XB(112), FX_WAIT, 6, FX_POS, XB(128), XB(36), FX_WAIT, 6
        DB FX_LOOP, 7, 40
        DB FX_HIDE
        DB FX_SIG, 1
        DB FX_END
FXSPARK2: DB FX_LOOK, FK_SINGLE, 4, SH_SPARK, 0, 0, C_YELLOW, 0
        DB FX_WAIT, 3
        DB FX_POS, XB(80), XB(136), FX_WAIT, 6, FX_POS, XB(22), XB(74), FX_WAIT, 6
        DB FX_POS, XB(132), XB(152), FX_WAIT, 6, FX_POS, XB(102), XB(24), FX_WAIT, 6
        DB FX_POS, XB(44), XB(164), FX_WAIT, 6, FX_POS, XB(152), XB(60), FX_WAIT, 6
        DB FX_POS, XB(8), XB(104), FX_WAIT, 6, FX_POS, XB(88), XB(94), FX_WAIT, 6
        DB FX_LOOP, 7, 40
        DB FX_HIDE
        DB FX_END

; Radiation: the plot's M.U.L.E. flashes, then bolts off to the right.
FXRADIA: DB FX_LOOK, FK_SINGLE, 3, SH_MULE, 2, 0, C_WHITE, 0
        DB FX_FRAME, 2, FX_WAIT, 6, FX_HIDE, FX_WAIT, 6
        DB FX_LOOP, 5, 7
        DB FX_FRAME, 2
        DB FX_MOVE, 2, 0, 1, 90
        DB FX_HIDE
        DB FX_SIG, 1
        DB FX_END

; Meteorite: falls from the top right onto its plot (the start set first).
FXMETEOR: DB FX_LOOK, FK_SINGLE, 3, SH_METEOR, 0, 0, C_ORANGE, 0
        DB FX_FRAME, 0
        DB FX_MOVE, SB(-2), 2, 2, 60
        DB FX_HIDE
        DB FX_SIG, 1
        DB FX_END

; The summary's walk-ins: each player enters 30 frames after the one before,
; walks left to x 7 (34 steps of 4: status.c's 33 would stand them on the
; money column), then climbs to its row, 4 lines every 6 frames (SU_SCRIPT).
SUSCR   MACRO w, n
        DB FX_WAIT, w, FX_MOVE, SB(-4), 0, 6, 34, FX_FRAME, 8
        DB FX_MOVE, 0, SB(-4), 6, n, FX_END
        ENDM
FXSUW0: SUSCR 1, 30
FXSUW1: SUSCR 30, 22
FXSUW2: SUSCR 60, 14
FXSUW3: SUSCR 90, 6
FXSUWT: DW FXSUW0, FXSUW1, FXSUW2, FXSUW3
