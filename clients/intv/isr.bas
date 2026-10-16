' ===========================================================================
' isr.bas -- the per-frame handler (ON FRAME GOSUB frame_isr).
'
' The main loop spends several frames at a time blocked in fujinet.bas's
' mailbox waits, so everything that has to move smoothly runs here instead:
' input sampling, our own walking, the land grant cursor, auction sprites and
' the scripted animations (fx.bas). The handler only writes MOBs (SPRITE) and
' its own variables (prefixed i-, w-, g-, au-, fx-); the main loop owns the
' screen (BACKTAB) and DEFINE. The handler's one GRAM write is the walking
' player's frame, through DEFINE ALTERNATE.
'
' Timings are the Atari's, frame for frame (docs/ANIMATION.md): both
' machines run at 60 Hz, one Atari colour clock is one pixel across, and one
' Intellivision pixel is two Atari scanlines down.
' ===========================================================================

    ' MOB assignment on the map and town screens.
    CONST MOB_PL = 0            ' player: 0 = top half, 1 = bottom half
    CONST MOB_MULE = 2
    CONST MOB_WAMPUS = 3        ' also the land grant cursor
    CONST MOB_RIVER = 4         ' 4, 5, 6: the river
    CONST MOB_FX = 7

    ' GRAM slots shared by the map and town screens (the map's own cards are
    ' 0..33; the town's are 0..TOWN_NCARDS-1, at most 40).
    CONST G_BAR = 40            ' 8 time bar cards
    CONST G_PL = 48             ' 4 cards: the walking player's current frame
    CONST G_MULE = 52           ' 2 cards: M.U.L.E. walk frames
    CONST G_WAMPUS = 54         ' 2 cards: wampus / grant cursor (8x16)
    CONST G_FX = 56             ' 8 cards for scripted sprites

    ' Walk events for the main loop (w_event).
    CONST WE_NONE = 0
    CONST WE_TOWN_LEFT = 1      ' walked out of town, left side
    CONST WE_TOWN_RIGHT = 2
    CONST WE_STORE_LEFT = 3     ' walked into the store from the left
    CONST WE_STORE_RIGHT = 4
    CONST WE_BUILDING = 5       ' walked into building w_bldg

    ' Buildings (w_bldg): the outfitters are GOOD_* so the shop is the good.
    CONST B_FOOD = 0
    CONST B_ENERGY = 1
    CONST B_SMITHORE = 2
    CONST B_CRYSTITE = 3
    CONST B_ASSAY = 4
    CONST B_LAND = 5
    CONST B_PUB = 6
    CONST B_CORRAL = 7

    ' Town geometry in MOB coordinates (docs/ANIMATION.md: Atari HPOS - 40).
    CONST TOWN_Y_TOP = 37       ' street: player top 37..41
    CONST TOWN_Y_BOT = 41
    CONST TOWN_Y_IN_DN = 57     ' walked into a lower building
    CONST TOWN_Y_IN_UP = 21     ' walked into an outfitter
    CONST TOWN_X_LEFT = 36      ' one more step left leaves town
    CONST TOWN_X_RIGHT = 132
    CONST TOWN_X_START = 84

    ' Walking state (MOB coordinates: x 8 and y 8 are the top-left pixel).
    DIM w_x, w_y, w_town, w_hold, w_event, w_bldg
    DIM w_cnt, w_step, w_frame, w_face, w_sp, w_col   ' w_col: colour 0..15
    DIM w_in                        ' walking into a building: 1 down, 2 up
    DIM w_mule                      ' 1 = leading a M.U.L.E.
    DIM w_tx(4), w_ty(4), w_th      ' the M.U.L.E. follows 4 steps behind
    DIM #w_gram                     ' queued species frame: offset + 1 ($8000: map figure)
    DIM t_l, t_h, t_m, t_p, t_k     ' walk_terrain (terrain.bas)

    ' Grant cursor.
    DIM g_cur, g_next, g_step, g_show

    ' Auction sprites (see auction.bas).
    DIM au_on, au_tick, au_stepv
    DIM #au_pos(4), #au_tgt(4)
    DIM au_role(4)                  ' ROLE_* per seat (auction.bas)
    DIM au_kind                     ' 1 land, 2 goods
    DIM #au_base                    ' price drawn at AU_Y_BASE (scrolls in land auctions)

frame_isr: PROCEDURE
    GOSUB read_input
    IF anim = AN_WALK THEN GOSUB isr_walk
    IF anim = AN_GRANT THEN GOSUB isr_grant
    IF anim = AN_AUCTION THEN GOSUB isr_auction
    ' The walk frame after the step: the picture and the MOBs both land at
    ' the next vertical blank.
    IF #w_gram THEN
        IF #w_gram AND $8000 THEN
            DEFINE ALTERNATE G_PL, 1, VARPTR gfx_mapsp((#w_gram AND $7FFF) - 1)
        ELSE
            DEFINE ALTERNATE G_PL, 4, VARPTR gfx_species(#w_gram - 1)
        END IF
        #w_gram = 0
    END IF
    IF fx_on THEN GOSUB fx_run
    GOSUB snd_isr
END

' ---------------------------------------------------------------------------
' Walking. One step every 6 frames (map: 14 on the river, 21 in the
' mountains); a step is 4 px across / 2 px down in town, 2 px / 1 px on the
' map. The walk frame flips on every step. On the map the player is the
' Atari's small figure (one 8x8 card at 0.5x), in town the full species.
' ---------------------------------------------------------------------------
isr_walk: PROCEDURE
    IF w_hold THEN RETURN
    IF w_cnt > 0 THEN
        IF t_l < 4 AND w_town = 0 THEN GOSUB walk_terrain     ' (terrain.bas)
        w_cnt = w_cnt - 1 : RETURN
    END IF
    IF w_in THEN GOSUB walk_building : RETURN
    ix = 0 : iy = 0
    IF inp_dir AND 8 THEN ix = 255      ' disc bits: 8 W, 2 E, 4 N, 1 S
    IF inp_dir AND 2 THEN ix = 1
    IF inp_dir AND 4 THEN iy = 255
    IF inp_dir AND 1 THEN iy = 1
    IF ix = 0 AND iy = 0 THEN
        IF w_step THEN w_step = 0 : w_face = 5 : GOSUB walk_frame
        RETURN
    END IF

    IF w_town THEN GOSUB walk_town ELSE GOSUB walk_map
    IF w_hold THEN RETURN
    w_step = 1
    w_frame = w_frame XOR 1
    IF ix = 1 THEN w_face = w_frame
    IF ix = 255 THEN w_face = 2 + w_frame
    IF ix = 0 AND iy = 1 THEN w_face = 6 + w_frame
    IF ix = 0 AND iy = 255 THEN w_face = 8 + w_frame
    GOSUB walk_frame
    t_l = 0 : t_h = 0               ' the terrain check starts next frame
    ' Remember where we were for the M.U.L.E. to follow.
    w_tx(w_th) = w_x : w_ty(w_th) = w_y
    w_th = (w_th + 1) AND 3
    GOSUB walk_draw
END

walk_frame: PROCEDURE
    IF w_town THEN
        #w_gram = (w_sp * 11 + w_face) * 16 + 1
    ELSE
        #w_gram = (w_sp * 10 + w_face) * 4 + $8001
    END IF
END

' walk_draw: put the player (and the M.U.L.E. four steps behind) on screen.
walk_draw: PROCEDURE
    #iw = $0800 + G_PL * 8 + (w_col AND 7) + (w_col AND 8) * $200
    IF w_town THEN
        SPRITE MOB_PL, w_x + VISIBLE, w_y + DOUBLEY, #iw
        SPRITE MOB_PL + 1, w_x + VISIBLE, (w_y + 8) + DOUBLEY, #iw + 16
    ELSE
        SPRITE MOB_PL, w_x + VISIBLE, w_y, #iw
        SPRITE MOB_PL + 1, 0, 0, 0
    END IF
    IF w_mule THEN
        ii = w_th                   ' the oldest remembered step
        IF w_tx(ii) = 0 THEN ii = 0
        IF w_tx(ii) THEN
            ' Black on the map; white in town, where the street is black.
            ' In town it stands as tall as the player; on the map it is
            ' drawn at half height to go with the small figure.
            #iw = $0800 + (G_MULE + w_frame) * 8 + w_town * SPR_WHITE
            IF w_town THEN
                IF w_face = 2 OR w_face = 3 THEN
                    SPRITE MOB_MULE, w_tx(ii) + VISIBLE, (w_ty(ii) + 3) + ZOOMY2 + FLIPX, #iw
                ELSE
                    SPRITE MOB_MULE, w_tx(ii) + VISIBLE, (w_ty(ii) + 3) + ZOOMY2, #iw
                END IF
            ELSE
                IF w_face = 2 OR w_face = 3 THEN
                    SPRITE MOB_MULE, w_tx(ii) + VISIBLE, w_ty(ii) + FLIPX, #iw
                ELSE
                    SPRITE MOB_MULE, w_tx(ii) + VISIBLE, w_ty(ii), #iw
                END IF
            END IF
        END IF
    ELSE
        SPRITE MOB_MULE, 0, 0, 0
    END IF
END

walk_town: PROCEDURE
    w_cnt = 5
    ' Along the street: 4 px a step. Out of either end: back to the map.
    IF ix = 255 THEN
        IF w_x <= TOWN_X_LEFT THEN w_event = WE_TOWN_LEFT : w_hold = 1 : RETURN
        w_x = w_x - 4
    ELSEIF ix = 1 THEN
        IF w_x >= TOWN_X_RIGHT THEN w_event = WE_TOWN_RIGHT : w_hold = 1 : RETURN
        w_x = w_x + 4
    END IF
    ' Up and down: the street is two steps deep. Pushing on past it at a
    ' door walks you in; the outfitters let you in only with a M.U.L.E.
    IF iy = 1 THEN
        IF w_y < TOWN_Y_BOT THEN
            w_y = w_y + 2
        ELSE
            FOR ii = 4 TO 7
                IF w_x >= town_door_lo(ii) AND w_x <= town_door_hi(ii) THEN w_in = 1 : w_bldg = ii
            NEXT ii
        END IF
    ELSEIF iy = 255 THEN
        IF w_y > TOWN_Y_TOP THEN
            w_y = w_y - 2
        ELSEIF w_mule THEN
            FOR ii = 0 TO 3
                IF w_x = town_door_lo(ii) THEN w_in = 2 : w_bldg = ii
            NEXT ii
        END IF
    END IF
END

' Walking through a door: 2 px a step until inside, then the main loop takes
' over (it runs the building's action and puts us back on the street).
walk_building: PROCEDURE
    w_cnt = 5
    w_frame = w_frame XOR 1
    IF w_in = 1 THEN
        w_y = w_y + 2 : w_face = 6 + w_frame
        IF w_y >= TOWN_Y_IN_DN THEN w_in = 0 : w_event = WE_BUILDING : w_hold = 1
    ELSE
        w_y = w_y - 2 : w_face = 8 + w_frame
        IF w_y <= TOWN_Y_IN_UP THEN w_in = 0 : w_event = WE_BUILDING : w_hold = 1
    END IF
    GOSUB walk_frame
    GOSUB walk_draw
END

' w_x, w_y are the Atari's (HPOS - 40, player line / 2 - 6.5): the small
' figure's top-left. Its middle, (w_x + 4, w_y + 2), is the point that picks
' the plot. The Atari keeps it to HPOS 55..197, lines 45..197.
walk_map: PROCEDURE
    w_cnt = 5                       ' walk_terrain sets it after the step
    IF ix = 255 AND w_x > 15 THEN w_x = w_x - 2
    IF ix = 1 AND w_x < 157 THEN w_x = w_x + 2
    IF iy = 255 AND w_y > 16 THEN w_y = w_y - 1
    IF iy = 1 AND w_y < 92 THEN w_y = w_y + 1
    ' Into the store (plot 22 is MOB x 80..95, y 48..63): the figure is
    ' columns 2..5 and 3.5 px tall.
    IF w_x + 5 >= 80 AND w_x + 2 <= 95 AND w_y + 3 >= 48 AND w_y <= 63 THEN
        IF w_x < 88 THEN w_event = WE_STORE_LEFT ELSE w_event = WE_STORE_RIGHT
        w_hold = 1
    END IF
END

' Doors, from the Atari's HPOS positions (docs/ANIMATION.md): the four
' outfitters (food, energy, smithore, crystite) and assay, land, pub, corral.
' The outfitters are indexed by good (food, energy, smithore, crystite);
' on the Atari they stand crystite, smithore, energy, food from the left.
town_door_lo: DATA 120, 96, 72, 48, 48, 68, 88, 108
town_door_hi: DATA 120, 96, 72, 48, 48, 68, 88, 124

' ---------------------------------------------------------------------------
' Land grant cursor: steps to the next free plot when its countdown runs
' out, blanking for one frame at each step, like the Atari (grant.bas syncs it).
' ---------------------------------------------------------------------------
    CONST GR_LIM = $8811        ' the furthest plot the cursor may step to
    CONST GR_PRESS = $8810      ' the cursor's plot when the button went down
                                ' (255 none): the main loop may be stuck in a
                                ' poll for a dozen frames before it sees the
                                ' press, and the cursor moves every 12
    CONST GR_WAS = $8813        ' the plot before the cursor's last step (255
                                ' none): a press reaches us a frame late (the
                                ' debounce), and the MOB we set shows a frame
                                ' later, so a press seen in the frame after a
                                ' step was made with the plot before on screen
    CONST GR_WANT = $8812       ' the plot we claimed and are waiting on (255
                                ' none)

' isr_grant (every frame): the cursor steps to the next free plot when its
' countdown runs out, blanking for that frame, like the Atari.
isr_grant: PROCEDURE
    IF btn_latch THEN
        IF g_show THEN
            IF g_cur < 45 THEN
                IF PEEK(GR_PRESS) AND 128 THEN
                    POKE GR_PRESS, g_cur
                    IF g_next = g_step AND (PEEK(GR_WAS) AND 255) < 45 THEN POKE GR_PRESS, PEEK(GR_WAS)
                END IF
            END IF
        END IF
    END IF
    IF g_next THEN
        IF PEEK(GR_WANT) AND 128 THEN
            g_next = g_next - 1
            IF g_next = 0 THEN
                ii = g_cur
                WHILE ii <> 45
                    IF ii = 255 THEN ii = 0 ELSE ii = ii + 1
                    IF ii <> STORE_PLOT THEN IF PEEK(STATE + ST_MAP_OWNER + ii) AND 128 THEN EXIT WHILE
                WEND
                IF ii <= (PEEK(GR_LIM) AND 255) THEN
                    POKE GR_WAS, g_cur
                    g_cur = ii : g_next = g_step
                    SPRITE MOB_WAMPUS, 0, 0, 0
                    RETURN
                END IF
                g_next = 1      ' a computer stops it here: the state says when
            END IF
        END IF
    END IF
    IF g_cur >= 45 OR g_show = 0 THEN
        ' (while the ship leaves, actor 0, MOB 3 is a quarter of it: fx.bas)
        IF (PEEK(FXB + AF_ST) AND 255) = 0 THEN SPRITE MOB_WAMPUS, 0, 0, 0
        RETURN
    END IF
    ix = (g_cur % 9) * 16 + 16
    iy = (g_cur / 9) * 16 + 16
    SPRITE MOB_WAMPUS, ix + VISIBLE + ZOOMX2, iy + DOUBLEY + ZOOMY2, $0800 + G_WAMPUS * 8 + SPR_BLACK
END

' ---------------------------------------------------------------------------
' Auction: every line moves one price step towards its target every 8
' frames (docs/TIMING.md). auction.bas turns prices into sprite rows.
' ---------------------------------------------------------------------------
isr_auction: PROCEDURE
    IF au_on = 0 THEN RETURN
    au_tick = au_tick + 1
    IF au_tick < 8 THEN RETURN
    au_tick = 0
    FOR ii = 0 TO 3
        IF #au_pos(ii) < #au_tgt(ii) THEN
            #au_pos(ii) = #au_pos(ii) + au_stepv
            IF #au_pos(ii) > #au_tgt(ii) THEN #au_pos(ii) = #au_tgt(ii)
        ELSEIF #au_pos(ii) > #au_tgt(ii) THEN
            #au_pos(ii) = #au_pos(ii) - au_stepv
            IF #au_pos(ii) < #au_tgt(ii) THEN #au_pos(ii) = #au_tgt(ii)
        END IF
        ' Land: a bid past 30 steps over the base scrolls the scale up, the
        ' high bidder staying at the top and the rest sliding down (Atari).
        IF au_kind = 1 AND au_role(ii) = ROLE_BUYER THEN
            #iw = au_stepv * 30
            IF #au_pos(ii) > #au_base + #iw THEN #au_base = #au_pos(ii) - #iw
        END IF
    NEXT ii
    GOSUB au_draw
END
