' ===========================================================================
' events.bas -- the set pieces: ships, colony events, production.
'
' Speeds and lengths are the Atari's (docs/ANIMATION.md). The production
' phase lasts ProductionDisplay (1421 jiffies) on the server, which is what
' the Atari takes; the pirate ship's slow legs are shortened to fit it.
'
' Coordinates are MOB coordinates. A plot p is at x = (p MOD 9) * 16 + 16,
' y = (p / 9) * 16 + 16. The store (plot 22) is x 80..95, y 48..63.
' ===========================================================================

    ' FX card slots on the map (G_FX = 56..63).
    CONST G_FX_NOSE = 56        ' 2 cards (8x16)
    CONST G_FX_BODY = 58        ' 4 cards
    CONST G_FX_A = 62           ' 2 cards: pest / bolt / meteor / spark / cloud

    CONST SHIP_Y = 45           ' body top in MOB y (Atari line 103)
    CONST PIRATE_Y = SHIP_Y - 4 ' the pirate's hull is drawn 4 px lower in its cards

    ' Word helpers for scripts: a SPRITE f word for GRAM card s, colour c.
    ' (DATA can't call DEF FN, so these are spelled out as constants.)
    CONST F_NOSE_T = $0800 + G_FX_NOSE * 8 + 5 + $1000     ' light blue (13)
    CONST F_NOSE_P = $0800 + G_FX_NOSE * 8 + 4 + $1000     ' pink (12)
    CONST D_BODY = G_FX_BODY - G_FX_NOSE                   ' body cards after the nose
    CONST F_A_BLACK = $0800 + G_FX_A * 8 + 0
    CONST F_A_WHITE = $0800 + G_FX_A * 8 + 7
    CONST F_A_YELLOW = $0800 + G_FX_A * 8 + 6
    CONST F_A_GREY = $0800 + G_FX_A * 8 + 0 + $1000         ' colour 8
    CONST F_A_BLUE = $0800 + G_FX_A * 8 + 1
    CONST F_A_LTBLUE = $0800 + G_FX_A * 8 + 5 + $1000
    CONST F_A_ORANGE = $0800 + G_FX_A * 8 + 2 + $1000
    CONST F_MULE_BLK = $0800 + G_MULE * 8 + 0
    CONST F_B_YELLOW = $0800 + G_FX_NOSE * 8 + 6       ' lightning (in the nose's slots)
    ' Docking frames: four of the Atari's eight, loaded into G_PL..G_PL+7
    ' (free while nobody walks). Light blue.
    CONST F_DOCK0 = $0800 + (G_PL + 0) * 8 + 5 + $1000
    CONST F_DOCK1 = $0800 + (G_PL + 2) * 8 + 5 + $1000
    CONST F_DOCK2 = $0800 + (G_PL + 4) * 8 + 5 + $1000
    CONST F_DOCK3 = $0800 + (G_PL + 6) * 8 + 5 + $1000

    ' Colours for the ship's halves (LOOK's xattr/yattr high bytes).
    CONST TOP_T = C_LTBLUE * 256
    CONST BOT_T = C_BLUE * 256
    CONST TOP_P = C_PINK * 256
    CONST BOT_P = C_RED * 256

' ---------------------------------------------------------------------------
' Scripts.
' ---------------------------------------------------------------------------
' Transport ship landing (round 1 arrival, round 12 "the ship is back").
fx_land:
    DATA FX_LOOK, FK_SHIP, 7, F_NOSE_T, D_BODY, TOP_T, BOT_T
    DATA FX_POS, 190, SHIP_Y
    DATA FX_WAIT, 36
    DATA FX_MOVE, -1, 0, 1, 230         ' fast pass right to left, 1 px a frame
    DATA FX_HIDE, FX_WAIT, 18
    DATA FX_POS, -32, SHIP_Y
    DATA FX_MOVE, 1, 0, 6, 78           ' slow return, 1 px every 6 frames
    DATA FX_MOVE, 5, 0, 6, 9            ' settling onto the store
    DATA FX_HIDE
    DATA FX_LOOK, FK_SINGLE, 7, F_DOCK0, 0, 0, 1          ' sinking into it
    DATA FX_POS, 84, 49, FX_WAIT, 8
    DATA FX_CARD, F_DOCK1, FX_WAIT, 8, FX_CARD, F_DOCK2, FX_WAIT, 8
    DATA FX_CARD, F_DOCK3, FX_WAIT, 8, FX_HIDE
    DATA FX_SIG, 1
    DATA FX_END

' Transport ship leaving (after round 1's summary; "back in 12 months").
fx_depart:
    DATA FX_WAIT, 45
    DATA FX_LOOK, FK_SINGLE, 7, F_DOCK3, 0, 0, 1          ' rising out of the store
    DATA FX_POS, 84, 49, FX_WAIT, 8
    DATA FX_CARD, F_DOCK2, FX_WAIT, 8, FX_CARD, F_DOCK1, FX_WAIT, 8
    DATA FX_CARD, F_DOCK0, FX_WAIT, 8, FX_HIDE
    DATA FX_LOOK, FK_SHIP, 7, F_NOSE_T, D_BODY, TOP_T, BOT_T
    DATA FX_POS, 93, SHIP_Y
    DATA FX_MOVE, -5, 0, 8, 9           ' rising out of the store
    DATA FX_MOVE, -1, 0, 8, 90          ' off to the left, 1 px every 8 frames
    DATA FX_HIDE
    DATA FX_SIG, 1
    DATA FX_END

' Pirate ship: fast pass, back to the store, raid, away.
fx_pirate:
    DATA FX_LOOK, FK_SHIP, 7, F_NOSE_P, D_BODY, TOP_P, BOT_P
    DATA FX_POS, 190, PIRATE_Y
    DATA FX_WAIT, 36
    DATA FX_MOVE, -1, 0, 1, 230
    DATA FX_HIDE, FX_WAIT, 18
    DATA FX_POS, -32, PIRATE_Y
    DATA FX_MOVE, 1, 0, 4, 78
    DATA FX_MOVE, 5, 0, 6, 9
    DATA FX_SIG, 2                      ' landed: "pirates take all crystite!"
    DATA FX_WAIT, 150
    DATA FX_MOVE, -5, 0, 8, 9
    DATA FX_MOVE, -1, 0, 4, 90
    DATA FX_HIDE
    DATA FX_SIG, 1
    DATA FX_END

' Pest: crawls around its plot (set AF_X/AF_Y first), 2 px every 5 frames,
' pausing 11 frames at each turn.
fx_pest:
    DATA FX_LOOK, FK_SINGLE, 3, F_A_BLACK, 0, 0, 1        ' 8x16, 0.5x
    DATA FX_CARD, F_A_BLACK
    DATA FX_MOVE, 2, 0, 5, 3, FX_WAIT, 11
    DATA FX_MOVE, 0, 1, 5, 3, FX_WAIT, 11
    DATA FX_MOVE, -2, 0, 5, 3, FX_WAIT, 11
    DATA FX_MOVE, 0, -1, 5, 3, FX_WAIT, 11
    DATA FX_LOOP, 6, 28
    DATA FX_HIDE
    DATA FX_SIG, 1
    DATA FX_END

' Acid rain cloud: drifts along its row (set AF_X/AF_Y), colour cycling.
' (The Atari's dark grey is black here: the map is the only grey.)
fx_cloud:
    DATA FX_LOOK, FK_SINGLE, 3, F_A_BLACK, 0, 4, 4          ' 2x, double width
    DATA FX_CARD, F_A_BLACK, FX_MOVE, 1, 0, 4, 5
    DATA FX_CARD, F_A_BLUE, FX_MOVE, 1, 0, 4, 5
    DATA FX_CARD, F_A_BLACK, FX_MOVE, 1, 0, 4, 5
    DATA FX_CARD, F_A_LTBLUE, FX_MOVE, 1, 0, 4, 5
    DATA FX_LOOP, 7, 28
    DATA FX_HIDE
    DATA FX_SIG, 1
    DATA FX_END
' Its lightning, under it, flickering: the same path (set 12 px lower).
fx_bolt:
    DATA FX_LOOK, FK_SINGLE, 7, F_B_YELLOW, 0, 0, 1        ' 8x16, 0.5x
    DATA FX_CARD, F_B_YELLOW, FX_MOVE, 1, 0, 4, 5
    DATA FX_HIDE, FX_MOVE, 1, 0, 4, 5
    DATA FX_CARD, F_B_YELLOW, FX_MOVE, 1, 0, 4, 5
    DATA FX_HIDE, FX_MOVE, 1, 0, 4, 5
    DATA FX_LOOP, 7, 24
    DATA FX_HIDE
    DATA FX_END

' Sunspot sparks: single pixels flickering around the map.
fx_sparks:
    DATA FX_LOOK, FK_SINGLE, 3, F_A_WHITE, 0, 0, 2
    DATA FX_POS, 40, 30, FX_WAIT, 6, FX_POS, 120, 70, FX_WAIT, 6
    DATA FX_POS, 72, 22, FX_WAIT, 6, FX_POS, 150, 50, FX_WAIT, 6
    DATA FX_POS, 20, 80, FX_WAIT, 6, FX_POS, 100, 33, FX_WAIT, 6
    DATA FX_POS, 60, 64, FX_WAIT, 6, FX_POS, 136, 26, FX_WAIT, 6
    DATA FX_LOOP, 7, 40
    DATA FX_HIDE
    DATA FX_SIG, 1
    DATA FX_END
fx_sparks2:
    DATA FX_LOOK, FK_SINGLE, 7, F_A_YELLOW, 0, 0, 2
    DATA FX_WAIT, 3
    DATA FX_POS, 88, 76, FX_WAIT, 6, FX_POS, 30, 45, FX_WAIT, 6
    DATA FX_POS, 140, 84, FX_WAIT, 6, FX_POS, 110, 20, FX_WAIT, 6
    DATA FX_POS, 52, 90, FX_WAIT, 6, FX_POS, 160, 38, FX_WAIT, 6
    DATA FX_POS, 16, 60, FX_WAIT, 6, FX_POS, 96, 55, FX_WAIT, 6
    DATA FX_LOOP, 7, 40
    DATA FX_HIDE
    DATA FX_END

' Radiation: the plot's M.U.L.E. flashes, then bolts off to the right
' (set AF_X/AF_Y on the plot).
fx_radiation:
    DATA FX_LOOK, FK_SINGLE, 2, F_MULE_BLK, 0, 0, 2
    DATA FX_CARD, F_MULE_BLK, FX_WAIT, 6, FX_HIDE, FX_WAIT, 6
    DATA FX_LOOP, 5, 7
    DATA FX_CARD, F_MULE_BLK
    DATA FX_MOVE, 2, 0, 1, 90
    DATA FX_HIDE
    DATA FX_SIG, 1
    DATA FX_END

' Meteorite: falls from the top right onto its plot (set the start).
fx_meteor:
    DATA FX_LOOK, FK_SINGLE, 3, F_A_ORANGE, 0, 0, 2
    DATA FX_CARD, F_A_ORANGE
    DATA FX_MOVE, -2, 1, 2, 60
    DATA FX_HIDE
    DATA FX_SIG, 1
    DATA FX_END

' ---------------------------------------------------------------------------
' Loading the sprites an event needs into G_FX (main loop).
' ---------------------------------------------------------------------------
fx_load_ship: PROCEDURE           ' k = 0 transport, 1 pirate
    g_slot = G_FX_NOSE : g_n = 2 : #g_src = VARPTR gfx_ship(k * 24) : GOSUB gram_load
    g_slot = G_FX_BODY : g_n = 4 : #g_src = VARPTR gfx_ship(k * 24 + 8) : GOSUB gram_load
    ' Docking frames 0, 2, 4, 6 of the eight (largest first).
    FOR j = 0 TO 3
        g_slot = G_PL + j * 2 : g_n = 2 : #g_src = VARPTR gfx_dock(j * 16) : GOSUB gram_load
    NEXT j
END

' ---------------------------------------------------------------------------
' The production phase.
' ---------------------------------------------------------------------------
    DIM pr_event, pr_plot, pr_started, pr_done

production_enter: PROCEDURE
    IF 1 THEN
        map_dots = 0
        GOSUB map_enter
        pr_event = st8(ST_COLONY_EVENT)
        pr_plot = st8(ST_COLONY_PLOT)
        pr_started = 0 : pr_done = 0
        fx_flag = 0
        IF pr_event >= 1 AND pr_event <= 9 THEN
            title_id = T_EVENT + pr_event - 1
            GOSUB title_show
            msg_row = ROW_MSG : #msg_col = C_TAN : msg_id = M_EVENT + pr_event - 1 : GOSUB msg_show
            GOSUB pr_event_start
        ELSE
            #val = round : title_id = T_PRODUCTION : GOSUB title_show
            pr_done = 1
        END IF
        ' Plots the event touches are drawn as they were until it strikes.
        GOSUB map_update
    END IF
END

pr_event_start: PROCEDURE
    fa = 0
    IF pr_event = CE_PIRATES THEN
        k = 1 : GOSUB fx_load_ship
        #addr = VARPTR fx_pirate(0) : GOSUB fx_start
    ELSEIF pr_event = CE_SHIPRETURNS THEN
        k = 0 : GOSUB fx_load_ship
        #addr = VARPTR fx_land(0) : GOSUB fx_start
    ELSEIF pr_event = CE_PEST THEN
        g_slot = G_FX_A : g_n = 2 : #g_src = VARPTR gfx_pest(0) : GOSUB gram_load
        POKE FXB + AF_X, (pr_plot % 9) * 16 + 16 + 64 : POKE FXB + AF_Y, (pr_plot / 9) * 16 + 16 + 32
        #addr = VARPTR fx_pest(0) : GOSUB fx_start
    ELSEIF pr_event = CE_ACIDRAIN THEN
        g_slot = G_FX_A : g_n = 1 : #g_src = VARPTR gfx_cloud(0) : GOSUB gram_load
        g_slot = G_FX_NOSE : g_n = 2 : #g_src = VARPTR gfx_bolt(0) : GOSUB gram_load
        POKE FXB + AF_X, 8 + 64 : POKE FXB + AF_Y, (pr_plot / 9) * 16 + 16 + 32
        #addr = VARPTR fx_cloud(0) : GOSUB fx_start
        fa = 1
        POKE FXB + AF_X + 1, 8 + 64 : POKE FXB + AF_Y + 1, (pr_plot / 9) * 16 + 16 + 32 + 12
        #addr = VARPTR fx_bolt(0) : GOSUB fx_start
    ELSEIF pr_event = CE_SUNSPOT THEN
        g_slot = G_FX_A : g_n = 1 : #g_src = VARPTR gfx_spark(0) : GOSUB gram_load
        #addr = VARPTR fx_sparks(0) : GOSUB fx_start
        fa = 1 : #addr = VARPTR fx_sparks2(0) : GOSUB fx_start
    ELSEIF pr_event = CE_RADIATION THEN
        POKE FXB + AF_X, (pr_plot % 9) * 16 + 16 + 8 + 64 : POKE FXB + AF_Y, (pr_plot / 9) * 16 + 16 + 4 + 32
        #addr = VARPTR fx_radiation(0) : GOSUB fx_start
    ELSEIF pr_event = CE_METEORITE THEN
        g_slot = G_FX_A : g_n = 1 : #g_src = VARPTR gfx_meteor(0) : GOSUB gram_load
        ' From 120 px right and 60 px up of the plot (clipped to the screen).
        #w = (pr_plot % 9) * 16 + 16 + 120 : IF #w > 190 THEN #w = 190
        POKE FXB + AF_X, #w + 64
        #w = (pr_plot / 9) * 16 + 16 + 32 : IF #w > 60 THEN #w = #w - 60 ELSE #w = 0
        POKE FXB + AF_Y, #w
        #addr = VARPTR fx_meteor(0) : GOSUB fx_start
    ELSEIF pr_event = CE_FIRE THEN
        g_slot = G_FX_A : g_n = 2 : #g_src = VARPTR gfx_fire(0) : GOSUB gram_load
        #ph_t0 = FRAME
    ELSEIF pr_event = CE_QUAKE THEN
        #ph_t0 = FRAME
    END IF
    pr_started = 1
    GOSUB snd_event
END

production_tick: PROCEDURE
    GOSUB msg_tick_run
    IF pr_done = 0 THEN
        IF pr_event = CE_FIRE THEN
            ' The store burns for 640 frames: fire cards flicker over it.
            IF FRAME - #ph_t0 < 640 THEN
                IF (FRAME AND 3) = 0 THEN
                    c = RAND AND 3
                    #w = gram_word(G_FX_A + (RAND AND 1), C_ORANGE + (RAND AND 1) * 4)
                    k = RAND AND 3
                    #BACKTAB((5 + k / 2) * 20 + 9 + (k AND 1)) = #w
                    mp = STORE_PLOT
                    IF c = 0 THEN GOSUB map_draw_plot
                END IF
            ELSE
                mp = STORE_PLOT : GOSUB map_draw_plot
                pr_done = 1
            END IF
        ELSEIF pr_event = CE_QUAKE THEN
            ' The ground shakes: the border and river jolt for 300 frames.
            IF FRAME - #ph_t0 < 300 THEN
                IF FRAME AND 2 THEN BORDER C_BLACK ELSE BORDER C_GREY
                #w = gram_word(MC_RIVER, RIVER_COL) + BEHIND
                k = 84 + (FRAME AND 4) / 2
                SPRITE MOB_RIVER, k + VISIBLE, 16 + DOUBLEY + ZOOMY4, #w
                SPRITE MOB_RIVER + 1, k + VISIBLE, 48 + DOUBLEY + ZOOMY4, #w
            ELSE
                BORDER C_GREY
                GOSUB river_show
                pr_done = 1
            END IF
        ELSEIF fx_flag = 1 OR (pr_started AND PEEK(FXB + AF_ST) = 0 AND PEEK(FXB + AF_ST + 1) = 0) THEN
            pr_done = 1
        END IF
        IF pr_done THEN
            GOSUB fx_stop_all
            GOSUB river_show
            #val = round : title_id = T_PRODUCTION : GOSUB title_show
            map_dots = 1
            GOSUB map_draw_all
        END IF
        RETURN
    END IF
    ' Production dots appear plot by plot.
    IF map_dots = 0 THEN
        map_dots = 1 : GOSUB map_draw_all
    END IF
END
