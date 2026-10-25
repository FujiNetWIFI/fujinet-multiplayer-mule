; prod.inc -- the production phase and its colony event (mekkogx events.c):
; the event's title, message and set piece on the map, then PRODUCTION and
; a dot a unit on every plot. The server shows it for 1421 jiffies.
;   pirates      the ship's fast pass, back over the store, the raid, away
;   pest         crawls round its plot           acid rain  cloud and bolt
;   planetquake  the picture shakes, plots jolt   sunspot   sparks, flashes
;   meteorite    falls onto its plot, a flash     radiation the M.U.L.E. runs
;   fire         flames over the store            the ship  lands (round 12)

PR_EV   EQU     PH+0
PR_PLOT EQU     PH+1
PR_ST   EQU     PH+2                    ; the set piece started
PR_DONE EQU     PH+3
PR_LAST EQU     PH+4                    ; the frame of the last flicker
PR_T0L  EQU     PH+5                    ; the frame it began
PR_T0H  EQU     PH+6
PR_X    EQU     PH+7                    ; the plot's corner
PR_Y    EQU     PH+8
PR_INV  EQU     PH+9                    ; the picture inverted now
FLAMEX  EQU     DOCKX
FLAMEY  EQU     MAPY0+33

; rand8: D = the next of a byte LFSR (never 0).
        RFIT    16
rand8:  GETV    SEED
        SHR
        BNF     rnd1
        XRI     0B8H
rnd1:   STR     RA
        BNZ     rnd9
        LDI     1
        STR     RA
rnd9:   FRET

; prod_enter: the map, the event's title and message, its set piece.
        RFIT    186
prod_enter:
        SETV    MAPDOTS, 0
        FCALL   map_enter
        SLD     C_EVENT
        PUTV    PR_EV
        SLD     C_PLOT
        SMI     NUM_PLOTS
        BNF     pre1
        LDI     0-NUM_PLOTS
pre1:   ADI     NUM_PLOTS
        PUTV    PR_PLOT
        SHL                             ; its corner
        ADI     LO(plotxy)
        PLO     R8
        LDI     HI(plotxy)
        ADCI    0
        PHI     R8
        LDA     R8
        PUTV    PR_X
        LDN     R8
        PUTV    PR_Y
        SETV    PR_ST, 0
        SETV    PR_DONE, 0
        SETV    PR_INV, 0
        SETV    FXFLAG, 0
        GETV    FCL
        PUTV    PR_T0L
        PUTV    PR_LAST
        GETV    FCH
        PUTV    PR_T0H
        SETV    V1L, 0
        SETV    V1H, 0
        GETV    PR_EV                   ; 1-9: an event
        BZ      pre2
        SMI     CE_SHIPRETURNS+1
        BDF     pre2
        GETV    PR_EV
        ADI     T_EVENT-1
        FCALL   title_show
        GETV    PR_EV
        ADI     M_EVENT-1
        FCALL   msg_show
        FJMP    pr_start
pre2:   SETV    PR_DONE, 1              ; none: the dots at once
        SETV    MAPDOTS, 1
        GETV    RND
        PUTV    V1L
        LDI     T_PRODUCT
        FJMP    title_show

; pr_start: the event's set piece (mekkogx pr_event_start).
        RFIT    227
pr_start:
        SETV    PR_ST, 1
        LDI     SND_EVENT
        FCALL   snd_play
        GETV    PR_EV
        XRI     CE_PIRATES
        BNZ     prs1
        LDR     R9, fx_pirate
        BR      prs0
prs1:   XRI     CE_PIRATES!CE_SHIPRETURNS
        BNZ     prs2
        LDR     R9, fx_land
prs0:   LDI     0
        FJMP    fx_start
prs2:   XRI     CE_SHIPRETURNS!CE_PEST
        BNZ     prs3
        GETV    PR_X                    ; the pest: round its plot
        PUTV    FXX
        GETV    PR_Y
        ADI     2
        PUTV    FXY
        LDR     R9, fx_pest
        BR      prs0
prs3:   XRI     CE_PEST!CE_ACIDRAIN
        BNZ     prs4
        SETV    FXX, 0F8H               ; the cloud along the plot's row, its
        GETV    PR_Y                    ; bolt under it
        ADI     1
        PUTV    FXY
        ADI     5
        PUTV    FXY+1
        SETV    FXX+1, 0FAH
        LDR     R9, fx_bolt
        LDI     1
        FCALL   fx_start
        LDR     R9, fx_cloud
        BR      prs0
prs4:   XRI     CE_ACIDRAIN!CE_SUNSPOT
        BNZ     prs5
        LDR     R9, fx_spark2
        LDI     1
        FCALL   fx_start
        LDR     R9, fx_spark1
        BR      prs0
prs5:   XRI     CE_SUNSPOT!CE_RADIATION
        BNZ     prs6
        GETV    PR_X                    ; the plot's M.U.L.E.
        ADI     2
        PUTV    FXX
        GETV    PR_Y
        ADI     4
        PUTV    FXY
        LDR     R9, fx_radiat
        BR      prs0
prs6:   XRI     CE_RADIATION!CE_METEORITE
        BNZ     prs9
        GETV    PR_X                    ; from up and to the right of it
        ADI     31
        PUTV    FXX
        GETV    PR_Y
        SMI     24
        PUTV    FXY
        LDR     R9, fx_meteor
        BR      prs0
prs9:   FRET                            ; fire, quake: the frame does them

; prod_tick: the set piece's frame; then the production dots (mekkogx
; production_tick).
        RFIT    125
prod_tick:
        FCALL   msg_tick
        GETV    PR_DONE
        BNZ     prt9
        GETV    PR_EV
        XRI     CE_FIRE
        BNZ     prt2
        LDI     HI(640)                 ; the store burns 640 frames
        PHI     RB
        LDI     LO(640)
        FCALL   pr_since
        BDF     prt8
        FJMP    pr_fire
prt2:   XRI     CE_FIRE!CE_QUAKE
        BNZ     prt3
        LDI     HI(300)                 ; the ground shakes 300
        PHI     RB
        LDI     LO(300)
        FCALL   pr_since
        BDF     prt8
        FJMP    pr_quake
prt3:   XRI     CE_QUAKE!CE_SUNSPOT     ; sunspots: the picture flashes
        BNZ     prt4
        FCALL   pr_flash
prt4:   GETV    FXFLAG                  ; the meteorite's impact: a flash
        XRI     3
        BNZ     prt5
        LDI     1
        FCALL   pr_view
prt5:   GETV    FXFLAG                  ; over: it says so, or nothing runs
        XRI     1
        BZ      prt8
        GETV    FXST
        STR     R2
        GETV    FXST+1
        OR
        ANI     FS_RUN
        BNZ     prt9
prt8:   FJMP    pr_end
prt9:   FRET

; pr_since: DF = the frames since the event began >= RB.1:D.
pr_since:
        PLO     RB
        GETV    PR_T0L
        STR     R2
        GETV    FCL
        SM
        PLO     R9
        GETV    PR_T0H
        STR     R2
        GETV    FCH
        SMB
        PHI     R9
        GLO     RB
        STR     R2
        GLO     R9
        SM
        GHI     RB
        STR     R2
        GHI     R9
        SMB
        FRET

; pr_end: the set piece gone, PRODUCTION, the dots (the map again).
        RFIT    81
pr_end:
        SETV    PR_DONE, 1
        FCALL   fx_stop_all
        PARS                            ; the flames
        P       0
        P       60H
        GOP     TOP_SPRHIDE
        LDI     0
        FCALL   pr_view
        SETV    MAPBLINK, NOBODY
        SETV    MAPBLON, 0
        GETV    RND
        PUTV    V1L
        SETV    V1H, 0
        LDI     T_PRODUCT
        FCALL   title_show
        SETV    MAPDOTS, 1
        SETV    MAPQ, 0
        FRET

; pr_view: the picture inverted (D = 1) or not, shaken by nothing.
        RFIT    34
pr_view:
        PUTV    PR_INV
        PARS
        P       0
        P       0
        GETV    PR_INV
        PD
        GOP     TOP_VIEW
        FRET

; pr_flash: sunspots: the picture inverted 3 frames in every 64.
        RFIT    33
pr_flash:
        GETV    PR_T0L
        STR     R2
        GETV    FCL
        SM
        ANI     3FH
        SMI     3
        LDI     0
        BDF     prf1
        LDI     1
prf1:   STR     R2
        GETV    PR_INV
        XOR
        BZ      prf9
        LDN     R2
        FJMP    pr_view
prf9:   FRET

; pr_fire: every 4 frames the flames over the store flicker (mekkogx: fire
; drawn over it at random).
        RFIT    126
pr_fire:
        FCALL   pr_every4
        BZ      pfi9
        FCALL   rand8
        PLO     RC
        ANI     1
        ADI     IM_FIRE0
        PLO     RB
        PARS
        P       13
        GLO     RB
        PD
        GLO     RC
        SHR
        ANI     1
        ADI     FLAMEX
        PD
        P       FLAMEY
        P       TM_OR
        GOP     TOP_SPRITE
        GLO     RC
        SHR
        SHR
        ANI     1
        ADI     IM_FIRE0
        PLO     RB
        PARS
        P       14
        GLO     RB
        PD
        GLO     RC
        SHR
        SHR
        SHR
        ANI     1
        ADI     FLAMEX+2
        PD
        P       FLAMEY+6
        P       TM_OR
        GOP     TOP_SPRITE
pfi9:   FRET

; pr_every4: D = 1 once every 4 frames.
pr_every4:
        GETV    PR_LAST
        STR     R2
        GETV    FCL
        SM
        SMI     4
        LDI     0
        BNF     pe49
        GETV    FCL
        PUTV    PR_LAST
        LDI     1
pe49:   FRET

; pr_quake: every 4 frames the picture jolts and a plot flashes outlined.
        RFIT    94
pr_quake:
        FCALL   pr_every4
        BZ      pqk9
        FCALL   rand8
        PLO     RC
        PARS                            ; dx -1..1, dy -2..1
        GLO     RC
        ANI     3
        SMI     1
        PD
        GLO     RC
        SHR
        SHR
        ANI     3
        SMI     2
        PD
        P       0
        GOP     TOP_VIEW
        GETV    MAPBLINK                ; the last plot back
        SMI     NUM_PLOTS
        BDF     pqk2
        GETV    MAPBLINK
        PLO     RB
        SETV    MAPBLINK, NOBODY
        GLO     RB
        FCALL   map_draw_plot
pqk2:   GLO     RC                      ; a plot: (k >> 2) mod 45
        SHR
        SHR
pqk3:   SMI     NUM_PLOTS
        BDF     pqk3
        ADI     NUM_PLOTS
        PUTV    MAPBLINK
        SETV    MAPBLON, 1
        GETV    MAPBLINK
        FJMP    map_draw_plot
pqk9:   FRET

; ---- the set pieces' scripts (mekkogx events.c; Studio II pixels, lines) ---------
PROD_SEGEND:
fx_pirate:
        DB      FX_LOOK, FK_SHIP, 5, IM_PSHIPL, 0, 0
        DB      FX_POS, 70, SHIPY
        DB      FX_WAIT, 36
        DB      FX_MOVE, -2, 0, 5, 46           ; the fast pass
        DB      FX_HIDE, FX_WAIT, 18
        DB      FX_POS, -16, SHIPY
        DB      FX_MOVE, 1, 0, 10, 30           ; back, faster than the ship
        DB      FX_MOVE, 2, 0, 6, 5             ; over the store
        DB      FX_SIG, 2
        DB      FX_WAIT, 150                    ; the raid
        DB      FX_MOVE, -1, 0, 4, 44           ; away to the left
        DB      FX_HIDE
        DB      FX_SIG, 1
        DB      FX_END

; the pest: round its plot (set its corner first), pausing at each turn
fx_pest:
        DB      FX_LOOK, FK_SINGLE, 13, IM_PEST0, 0, 1
        DB      FX_FRAME, 0
        DB      FX_MOVE, 1, 0, 5, 2, FX_WAIT, 11
        DB      FX_MOVE, 0, 2, 5, 4, FX_WAIT, 11
        DB      FX_MOVE, -1, 0, 5, 2, FX_WAIT, 11
        DB      FX_MOVE, 0, -2, 5, 4, FX_WAIT, 11
        DB      FX_LOOP, 6, 28
        DB      FX_HIDE, FX_SIG, 1, FX_END

; acid rain: the cloud drifts along the row, its bolt under it flickering
fx_cloud:
        DB      FX_LOOK, FK_SINGLE, 13, IM_CLOUD0, 0, 0
        DB      FX_FRAME, 0, FX_MOVE, 1, 0, 9, 4
        DB      FX_FRAME, 1, FX_MOVE, 1, 0, 9, 4
        DB      FX_LOOP, 8, 14
        DB      FX_HIDE, FX_SIG, 1, FX_END
fx_bolt:
        DB      FX_LOOK, FK_SINGLE, 14, IM_BOLT, 0, 0
        DB      FX_FRAME, 0, FX_MOVE, 1, 0, 9, 4
        DB      FX_HIDE, FX_MOVE, 1, 0, 9, 4
        DB      FX_LOOP, 8, 13
        DB      FX_HIDE, FX_END

; sunspots: sparks flickering about the map
fx_spark1:
        DB      FX_LOOK, FK_SINGLE, 13, IM_SPARK, 0, 0
        DB      FX_POS, 12, 22, FX_WAIT, 6, FX_POS, 44, 62, FX_WAIT, 6
        DB      FX_POS, 25, 14, FX_WAIT, 6, FX_POS, 56, 42, FX_WAIT, 6
        DB      FX_POS, 4, 72, FX_WAIT, 6, FX_POS, 36, 25, FX_WAIT, 6
        DB      FX_POS, 20, 56, FX_WAIT, 6, FX_POS, 51, 80, FX_WAIT, 6
        DB      FX_LOOP, 7, 40
        DB      FX_HIDE, FX_SIG, 1, FX_END
fx_spark2:
        DB      FX_LOOK, FK_SINGLE, 14, IM_SPARK, 0, 0
        DB      FX_WAIT, 3
        DB      FX_POS, 32, 68, FX_WAIT, 6, FX_POS, 9, 37, FX_WAIT, 6
        DB      FX_POS, 52, 86, FX_WAIT, 6, FX_POS, 41, 16, FX_WAIT, 6
        DB      FX_POS, 17, 82, FX_WAIT, 6, FX_POS, 58, 30, FX_WAIT, 6
        DB      FX_POS, 3, 52, FX_WAIT, 6, FX_POS, 30, 46, FX_WAIT, 6
        DB      FX_LOOP, 7, 40
        DB      FX_HIDE, FX_END

; radiation: the plot's M.U.L.E. flashes, then bolts off to the right
fx_radiat:
        DB      FX_LOOK, FK_SINGLE, 13, IM_SMULEA, 0, IM_SMULEB-IM_SMULEA
        DB      FX_FRAME, 0, FX_WAIT, 6, FX_HIDE, FX_WAIT, 6
        DB      FX_LOOP, 5, 7
        DB      FX_FRAME, 0
        DB      FX_MOVE, 1, 0, 2, 64
        DB      FX_HIDE, FX_SIG, 1, FX_END

; the meteorite: falls onto its plot (set its start first), then a flash
fx_meteor:
        DB      FX_LOOK, FK_SINGLE, 13, IM_METEOR, 0, 0
        DB      FX_FRAME, 0
        DB      FX_MOVE, -1, 1, 2, 30
        DB      FX_HIDE, FX_SIG, 3, FX_WAIT, 8
        DB      FX_SIG, 1, FX_END
