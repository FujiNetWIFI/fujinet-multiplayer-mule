' ===========================================================================
' develop.bas -- the development phase: turns, walking, the town.
'
' The server keeps no player position. We walk locally (isr.bas) and send
' actions: buy/return a M.U.L.E. at the corral, outfit it at a shop,
' install it on a plot, assay, list land, catch the wampus, gamble at the pub.
' A turn starts in the middle of town, as on the Atari.
' ===========================================================================

    DIM dv_seat, dv_mine, dv_state   ' (dv_flags: memory.bas)
    DIM dv_sample               ' plot we took an assay sample from (255 none)
    DIM dv_office               ' 1 = the land office is waiting for a plot
    DIM dv_msg_seat, dv_msg_kind ' the turn message showing: whose, and what
    CONST DV_AI = $8802         ' the computer's plot marks: 1 outline its land,
                                ' 2 shown (since DV_AIT), 0 done
    CONST DV_AIT = $8803        ' FRAME AND 255 when its land was outlined
    CONST DV_AIP = $8804        ' the plot it works on
    ' #dv_left (memory.bas): turn time left at the last poll (jiffies)
    DIM #dv_el                  ' turn elapsed at the last poll
    ' dv_ptu: memory.bas
    DIM dv_evshown              ' player event shown this turn
    DIM dv_wp                   ' wampus index showing (255 none)
    DIM wp_x, wp_y, wp_on       ' where it is, and whether it's drawn

    CONST DV_WAIT = 0           ' someone else's turn / between turns
    CONST DV_PROMPT = 1         ' our turn: press the button to start
    CONST DV_WALK = 2           ' walking
    CONST DV_OVER = 3           ' our turn has ended

' Called when the phase becomes develop, and on every new state in it.
develop_state: PROCEDURE
    dv_seat = st8(ST_TURN_SEAT)
    dv_flags = st8(ST_TURN_FLAGS)
    #dv_left = st16(ST_TURN_LEFT)
    #dv_el = st16(ST_TURN_ELAPSED)
    dv_ptu = st8(ST_TURN_P_TU)
    IF dv_ptu = 0 THEN dv_ptu = 28
    dv_mine = 0
    IF dv_seat = my_seat THEN dv_mine = 1

    IF dv_mine AND (dv_flags AND TF_STARTED) AND (dv_flags AND TF_ENDED) = 0 THEN
        IF dv_state <> DV_WALK THEN GOSUB dv_start_walk
        ' The server's view of what we lead wins over ours.
        IF dv_flags AND TF_HASMULE THEN w_mule = 1 ELSE w_mule = 0
        IF scr_now = SCR_MAP THEN
            GOSUB map_update
            GOSUB dv_map_result
        END IF
        RETURN
    END IF
    ' Anything else happens on the map.
    IF dv_state = DV_WALK THEN GOSUB dv_stop_walk
    IF scr_now <> SCR_MAP THEN
        GOSUB map_enter
        #val = round : title_id = T_DEVELOPMENT : GOSUB title_show
    END IF
    IF dv_mine AND (dv_flags AND TF_STARTED) = 0 THEN
        IF dv_state <> DV_PROMPT THEN
            dv_state = DV_PROMPT
            msg_row = ROW_MSG : #msg_col = C_BLACK : msg_id = M_START : GOSUB msg_show
            GOSUB snd_myturn
        END IF
    ELSEIF dv_seat < 4 THEN
        ' Someone's turn: say so once per turn, then once when it ends. A
        ' computer's turn is over on the server at once but plays out on the
        ' screen, so it stays "taking his turn" until the next seat starts.
        k = 0
        IF pl8(dv_seat, S_PLAYER_FLAGS) AND PF_AI THEN
            k = 1
        ELSEIF dv_flags AND TF_ENDED THEN
            k = 2
        END IF
        IF dv_seat <> dv_msg_seat OR k <> dv_msg_kind THEN
            dv_msg_seat = dv_seat : dv_msg_kind = k
            msg_row = ROW_MSG : #msg_col = C_BLACK
            IF k = 1 THEN
                dv_state = DV_WAIT
                #msg_col = pl_txtcol(pl8(dv_seat, S_PLAYER_COLOR) AND 3)
                msg_id = M_COMPUTER : GOSUB msg_show
                GOSUB dv_ai_walk
            ELSEIF k = 2 THEN
                dv_state = DV_OVER
                #msg_col = C_TAN
                IF act_sent = ACT_PUB AND dv_mine THEN
                    msg_id = M_GAMBLE : #val = st16(ST_RESULT) : GOSUB msg_show
                ELSEIF #dv_left = 0 THEN
                    msg_id = M_TIMEOUT : GOSUB msg_show
                ELSE
                    GOSUB msg_clear
                END IF
            ELSE
                dv_state = DV_WAIT
                ' "<NAME>'S TURN"
                #t_src = STATE + ST_PLAYERS + dv_seat * SZ_PLAYER
                FOR k = 0 TO 7
                    c = PEEK(#t_src + k) AND 255
                    IF c = 0 THEN EXIT FOR
                    IF c >= 97 THEN c = c - 32
                    POKE TXT + k, c
                NEXT k
                t_len = k
                #lit_addr = VARPTR t_msgs(0) : lit_n = M_TURN : GOSUB lit_nth
                FOR j = 1 TO 7
                    POKE TXT + t_len, PEEK(#lit_addr + j) + 32 : t_len = t_len + 1
                NEXT j
                GOSUB msg_start
            END IF
        END IF
    END IF
    ' Time bar for whoever is playing, in their colour.
    bar_col = 19 : bar_rows = 10
    IF dv_seat < 4 THEN bar_colr = pl_mobcol(pl8(dv_seat, S_PLAYER_COLOR) AND 3)
    IF (dv_flags AND TF_STARTED) AND (dv_flags AND TF_ENDED) = 0 THEN
        bar_px = #dv_left / dv_ptu / 2
    ELSE
        bar_px = 0
    END IF
    GOSUB bar_draw
    GOSUB map_update
END

' Per frame while in develop.
develop_tick: PROCEDURE
    GOSUB msg_tick_run
    GOSUB bar_tick_dv
    IF dv_state = DV_PROMPT THEN
        IF ev_btn THEN act = ACT_BEGIN : poll_wait = 0 : GOSUB snd_select
        RETURN
    END IF
    IF dv_state <> DV_WALK THEN
        IF dv_state = DV_WAIT THEN GOSUB dv_ai_marks
        ' Watching: run the bar down between polls.
        IF (dv_flags AND TF_STARTED) AND (dv_flags AND TF_ENDED) = 0 THEN
            #w = FRAME - #rx_jiffy
            IF #w < #dv_left THEN bar_px = (#dv_left - #w) / dv_ptu / 2 ELSE bar_px = 0
            GOSUB bar_draw
        END IF
        RETURN
    END IF

    ' Our own turn.
    #w = FRAME - #rx_jiffy
    IF #w < #dv_left THEN bar_px = (#dv_left - #w) / dv_ptu / 2 ELSE bar_px = 0
    GOSUB bar_draw
    GOSUB dv_event_msg
    IF w_event THEN GOSUB dv_walk_event : RETURN
    IF w_town = 0 THEN
        GOSUB dv_wampus
        IF ev_btn THEN GOSUB dv_button
    END IF
    IF ev_key = 11 THEN act = ACT_END : poll_wait = 0      ' ENTER: end turn
END

' The player event: shown scrolling for the start of the turn, as on the Atari.
dv_event_msg: PROCEDURE
    IF dv_evshown THEN RETURN
    dv_evshown = 1
    k = st8(ST_TURN_EVENT)
    IF k = 0 OR k > 22 THEN RETURN
    ' Amount: x * 25 * (round / 4 + 1), or per M.U.L.E. for three events.
    #val = 25 * (round / 4 + 1)
    #val2 = 0
    j = t_pevent_x(k - 1)
    IF j >= 100 THEN
        n = 0
        FOR mp = 0 TO 44
            IF st8(ST_MAP_OWNER + mp) = my_seat THEN
                mg = st8(ST_MAP_MULE + mp)
                IF j = 100 AND mg = GOOD_FOOD THEN n = n + 1
                IF j = 101 AND (mg = GOOD_SMITHORE OR mg = GOOD_CRYSTITE) THEN n = n + 1
                IF j = 102 AND mg = GOOD_ENERGY THEN n = n + 1
            END IF
        NEXT mp
        IF j = 102 THEN #val = #val * n ELSE #val2 = #val * 2 * n : #val = #val * 2
        IF j = 101 THEN #val = #val2
    ELSE
        #val = #val * j
    END IF
    #lit_addr = VARPTR t_pevents(0) : lit_n = k - 1 : GOSUB lit_nth
    #t_src = #lit_addr : GOSUB text_expand
    ' "*** " + text, scrolling
    FOR j = t_len TO 1 STEP -1
        POKE TXT + j + 3, PEEK(TXT + j - 1)
    NEXT j
    POKE TXT, 42 : POKE TXT + 1, 42 : POKE TXT + 2, 42 : POKE TXT + 3, 32
    t_len = t_len + 4
    IF w_town THEN msg_row = 200 ELSE msg_row = ROW_MSG
    #msg_col = C_RED
    GOSUB msg_start
END

' ---------------------------------------------------------------------------
dv_start_walk: PROCEDURE
    dv_state = DV_WALK
    dv_sample = 255 : dv_office = 0 : dv_evshown = 0
    w_sp = pl8(my_seat, S_PLAYER_SPECIES) AND 7
    w_col = pl_mobcol(pl8(my_seat, S_PLAYER_COLOR) AND 3)
    bar_colr = w_col
    FOR k = 0 TO 3 : w_tx(k) = 0 : w_ty(k) = 0 : NEXT k
    w_x = TOWN_X_START : w_y = TOWN_Y_TOP + 2
    GOSUB town_enter
    GOSUB snd_select
END

dv_stop_walk: PROCEDURE
    anim = AN_NONE
    w_hold = 1
    dv_state = DV_OVER
    SPRITE MOB_PL, 0, 0, 0 : SPRITE MOB_PL + 1, 0, 0, 0 : SPRITE MOB_MULE, 0, 0, 0
    SPRITE MOB_WAMPUS, 0, 0, 0
    IF scr_now <> SCR_MAP THEN
        GOSUB map_enter
        #val = round : title_id = T_DEVELOPMENT : GOSUB title_show
    END IF
END

' Town and map transitions: the screen blanks for about 22 frames on the Atari.
dv_walk_event: PROCEDURE
    IF w_event = WE_TOWN_LEFT OR w_event = WE_TOWN_RIGHT THEN
        IF w_event = WE_TOWN_LEFT THEN w_x = 68 ELSE w_x = 99
        w_y = 52
        w_event = 0
        w_town = 0
        GOSUB dv_blank
        GOSUB map_enter
        #val = round : title_id = T_DEVELOPMENT : GOSUB title_show
        GOSUB dv_resume
        w_cnt = 20 : t_l = 4        ' the first map step waits 20 frames
    ELSEIF w_event = WE_STORE_LEFT OR w_event = WE_STORE_RIGHT THEN
        IF w_event = WE_STORE_LEFT THEN w_x = TOWN_X_LEFT ELSE w_x = TOWN_X_RIGHT
        w_y = TOWN_Y_TOP + 2
        w_event = 0
        GOSUB dv_blank
        GOSUB town_enter
    ELSEIF w_event = WE_BUILDING THEN
        w_event = 0
        GOSUB dv_building
    END IF
END

dv_blank: PROCEDURE
    anim = AN_NONE
    SPRITE MOB_PL, 0, 0, 0 : SPRITE MOB_PL + 1, 0, 0, 0 : SPRITE MOB_MULE, 0, 0, 0
    SPRITE MOB_WAMPUS, 0, 0, 0
    FOR k = 0 TO 7 : SPRITE k, 0, 0, 0 : NEXT k
    FOR k = 1 TO 22 : WAIT : NEXT k
    FOR k = 0 TO 3 : w_tx(k) = 0 : NEXT k
END

dv_resume: PROCEDURE
    w_hold = 0
    w_face = 5 : GOSUB walk_frame
    GOSUB walk_draw
    anim = AN_WALK
END

' ---------------------------------------------------------------------------
' Buildings. The action goes to the server; then we step back out onto the
' street.
' ---------------------------------------------------------------------------
dv_building: PROCEDURE
    msg_row = 200 : #msg_col = C_WHITE + fgbg_bg(C_BLACK)
    IF w_bldg = B_CORRAL THEN
        IF w_mule THEN act = ACT_RETURN ELSE act = ACT_BUYMULE
    ELSEIF w_bldg <= B_CRYSTITE THEN
        act = ACT_OUTFIT : #act_arg = w_bldg
    ELSEIF w_bldg = B_ASSAY THEN
        IF dv_sample < 45 THEN
            act = ACT_ASSAY : #act_arg = dv_sample
        ELSE
            msg_id = M_NOSAMPLE : GOSUB msg_show
        END IF
    ELSEIF w_bldg = B_LAND THEN
        dv_office = 1
        msg_id = M_LANDOFFICE : GOSUB msg_show
    ELSEIF w_bldg = B_PUB THEN
        act = ACT_PUB
    END IF
    IF act THEN
        k = act
        poll_wait = 0
        GOSUB net_poll               ' do it now, while we stand in the door
        GOSUB dv_building_result
    END IF
    ' Back out onto the street.
    IF dv_state = DV_WALK THEN
        w_y = TOWN_Y_TOP + 2
        GOSUB snd_select
        GOSUB dv_resume
    END IF
END

dv_building_result: PROCEDURE
    IF net_have <> 1 THEN RETURN
    c = st8(ST_ERR)
    IF c = ERR_MONEY THEN msg_id = M_NOMONEY : GOSUB msg_show : RETURN
    IF c = ERR_NOSTOCK THEN msg_id = M_NOMULES : GOSUB msg_show : RETURN
    IF c THEN GOSUB snd_error : RETURN
    IF act_sent = ACT_ASSAY THEN
        msg_id = M_ASSAY + (st16(ST_RESULT) AND 7) : GOSUB msg_show
        dv_sample = 255
    END IF
    IF act_sent = ACT_BUYMULE THEN
        #val = st16(ST_STORE_MULE_PRICE) : GOSUB snd_buy
    END IF
    IF act_sent = ACT_OUTFIT THEN GOSUB snd_buy
    GOSUB town_money
END

' dv_ai_marks (per frame in a computer's turn): the Atari outlines the
' computer's land in white for 40 frames as its turn starts, and the plot it
' works on while it is there (the walker's FX_SIG 3 and 4).
dv_ai_marks: PROCEDURE
    k = PEEK(DV_AI) AND 255
    IF k = 1 THEN
        h_k = 1 : GOSUB dv_ai_land
        POKE DV_AI, 2 : POKE DV_AIT, FRAME
    ELSEIF k = 2 THEN
        IF ((FRAME - PEEK(DV_AIT)) AND 255) >= 40 THEN h_k = 0 : GOSUB dv_ai_land : POKE DV_AI, 0
    END IF
    IF fx_flag = 3 OR fx_flag = 4 THEN
        mp = PEEK(DV_AIP) AND 255
        IF fx_flag = 3 THEN map_blink = mp : map_blink_on = 1
        IF mp < 45 THEN GOSUB map_draw_plot
        map_blink = 255 : fx_flag = 0
    END IF
END

' dv_ai_land: draw the computer's plots outlined in white (h_k = 1) or
' normally (0).
dv_ai_land: PROCEDURE
    FOR mp = 0 TO 44
        IF st8(ST_MAP_OWNER + mp) = dv_seat THEN
            IF h_k THEN map_blink = mp : map_blink_on = 1
            GOSUB map_draw_plot
            map_blink = 255
        END IF
    NEXT mp
END

' ---------------------------------------------------------------------------
' A computer's turn: the server plays it at once, so the map already shows
' what it did. On the Atari the computer's figure stands in the store for
' its whole turn (HPOS 123, line 118: MOB 83, 52). Here it also walks out
' from the store to the plot that gained a M.U.L.E. (or, if none did, to
' one of its plots) with the M.U.L.E. four steps behind, which stays there,
' then walks back into the store and stands there until the next turn, at
' map walking speed. The walk is a script written into cartridge RAM for
' fx.bas.
' ---------------------------------------------------------------------------
    CONST FXS = $8650           ' 2 x 48 bytes of script (to $86AF)
dv_ai_walk: PROCEDURE
    GOSUB fx_stop_all
    j = 255 : n = 0
    FOR mp = 0 TO 44
        IF st8(ST_MAP_OWNER + mp) = dv_seat AND mp <> STORE_PLOT THEN
            IF j = 255 THEN j = mp
            IF st8(ST_MAP_MULE + mp) < 4 THEN
                IF (PEEK(STATE + ST_MAP_MULE + mp) XOR PEEK(PREV + ST_MAP_MULE + mp)) AND 255 THEN j = mp : n = 1
            END IF
        END IF
    NEXT mp
    IF j > 44 THEN j = 13                       ' no land: out and back
    ' From beside the store to the middle of the plot.
    x = (j % 9) * 16 + 16 + 4 : y = (j / 9) * 16 + 16 + 6
    c = 83                                      ' in the store
    ' The walker: its species' small map figure, walking right (flipped to
    ' go left).
    k = pl8(dv_seat, S_PLAYER_SPECIES) AND 7
    g_slot = G_FX : g_n = 2 : #g_src = VARPTR gfx_mapsp((k * 10) * 4) : GOSUB gram_load
    k = pl_mobcol(pl8(dv_seat, S_PLAYER_COLOR) AND 3)
    #fx_f(0) = $0800 + G_FX * 8 + (k AND 7) + (k AND 8) * $200
    POKE FXB + AF_KIND, FK_SINGLE : POKE FXB + AF_MOB, 0 : POKE FXB + AF_D2, 1
    POKE FXB + AF_XA, 0 : POKE FXB + AF_YA, 0
    POKE FXB + AF_X, c + 64 : POKE FXB + AF_Y, 52 + 32
    #fx_f(1) = $0800 + G_MULE * 8
    POKE FXB + AF_KIND + 1, FK_SINGLE : POKE FXB + AF_MOB + 1, 2 : POKE FXB + AF_D2 + 1, 1
    POKE FXB + AF_XA + 1, 0 : POKE FXB + AF_YA + 1, 0
    POKE FXB + AF_X + 1, c + 64 : POKE FXB + AF_Y + 1, 52 + 32
    ' Scripts: across 2 px every 6 frames, then down/up 1 px every 6, 90
    ' frames on the plot (outlined in white, as the Atari outlines the plot a
    ' computer works: FX_SIG 3 / 4 tell dv_ai_marks), and the same way back
    ' into the store, where the figure waits (a long loop, so fx_stop_all
    ' takes it off when the next turn starts). The M.U.L.E. stops on the
    ' plot. Facing (FLIPX) follows the direction.
    IF (PEEK(DV_AI) AND 255) = 2 THEN GOSUB map_draw_all    ' the last one's land
    POKE DV_AIP, j : POKE DV_AI, 1
    FOR fa = 0 TO 1
        IF fa = 0 OR n THEN
            #addr = FXS + fa * 48
            k = 0
            IF fa THEN POKE #addr, FX_WAIT : POKE #addr + 1, 24 : k = 2
            IF x >= c THEN
                POKE #addr + k, FX_YATTR : POKE #addr + k + 1, 0
                POKE #addr + k + 2, FX_MOVE : POKE #addr + k + 3, 2 : h_c = (x - c) / 2
            ELSE
                POKE #addr + k, FX_YATTR : POKE #addr + k + 1, 8
                POKE #addr + k + 2, FX_MOVE : POKE #addr + k + 3, 254 : h_c = (c - x) / 2
            END IF
            POKE #addr + k + 4, 0 : POKE #addr + k + 5, 6 : POKE #addr + k + 6, h_c + 1
            POKE #addr + k + 7, FX_MOVE : POKE #addr + k + 8, 0
            IF y >= 52 THEN POKE #addr + k + 9, 1 : h_i = y - 52 ELSE POKE #addr + k + 9, 255 : h_i = 52 - y
            POKE #addr + k + 10, 6 : POKE #addr + k + 11, h_i + 1
            IF fa THEN
                ' The M.U.L.E. is installed: it goes no further.
                POKE #addr + k + 12, FX_WAIT : POKE #addr + k + 13, 20
                POKE #addr + k + 14, FX_HIDE : POKE #addr + k + 15, FX_END
            ELSE
                POKE #addr + 12, FX_SIG : POKE #addr + 13, 3
                POKE #addr + 14, FX_WAIT : POKE #addr + 15, 90
                POKE #addr + 16, FX_SIG : POKE #addr + 17, 4
                ' ... and back: vertical first, then across, facing home.
                POKE #addr + 18, FX_MOVE : POKE #addr + 19, 0
                POKE #addr + 20, 256 - PEEK(#addr + 9) : POKE #addr + 21, 6 : POKE #addr + 22, h_i + 1
                POKE #addr + 23, FX_YATTR : POKE #addr + 24, (PEEK(#addr + 1) XOR 8)
                POKE #addr + 25, FX_MOVE : POKE #addr + 26, 256 - PEEK(#addr + 3)
                POKE #addr + 27, 0 : POKE #addr + 28, 6 : POKE #addr + 29, h_c + 1
                POKE #addr + 30, FX_WAIT : POKE #addr + 31, 255
                POKE #addr + 32, FX_LOOP : POKE #addr + 33, 200 : POKE #addr + 34, 2
            END IF
            GOSUB fx_start
        END IF
    NEXT fa
END

' What the server said about a map action we just sent.
dv_map_result: PROCEDURE
    IF act_sent < ACT_INSTALL OR act_sent > ACT_WAMPUS THEN RETURN
    msg_row = ROW_MSG : #msg_col = C_TAN : msg_id = 0
    IF st8(ST_ERR) THEN GOSUB snd_error : act_sent = 0 : RETURN
    IF act_sent = ACT_INSTALL THEN
        IF st16(ST_RESULT) = 1 THEN msg_id = M_RANAWAY : GOSUB snd_error ELSE GOSUB snd_buy
    ELSEIF act_sent = ACT_WAMPUS THEN
        msg_id = M_CAUGHT : GOSUB snd_founder
    ELSEIF act_sent = ACT_SELL THEN
        msg_id = M_FORSALE
    ELSEIF act_sent = ACT_UNSELL THEN
        msg_id = M_NOTFORSALE
    END IF
    IF msg_id THEN #val = st16(ST_RESULT) : GOSUB msg_show
    act_sent = 0
END

' The button on the map: install the M.U.L.E. we lead, catch the wampus,
' list land after the land office, or take an assay sample.
dv_button: PROCEDURE
    ' The plot under the middle of the small figure (w_x + 4, w_y + 2).
    mp = 255
    IF w_x >= 12 AND w_y >= 14 THEN
        x = (w_x - 12) / 16 : y = (w_y + 2 - 16) / 16
        IF x < 9 AND y < 5 THEN mp = y * 9 + x
    END IF
    IF dv_wp < 12 THEN
        ' Within 8 pixels of a showing wampus catches it.
        IF w_x + 8 >= wp_x AND w_x <= wp_x + 8 AND w_y + 4 >= wp_y AND w_y <= wp_y + 8 THEN
            IF w_mule = 0 THEN act = ACT_WAMPUS : poll_wait = 0 : RETURN
        END IF
    END IF
    IF mp >= 45 THEN RETURN
    IF w_mule THEN
        act = ACT_INSTALL : #act_arg = mp : poll_wait = 0
        GOSUB snd_select
        RETURN
    END IF
    IF dv_office THEN
        IF st8(ST_MAP_OWNER + mp) = my_seat THEN
            IF st8(ST_MAP_PROD + mp) AND 64 THEN act = ACT_UNSELL ELSE act = ACT_SELL
            #act_arg = mp : poll_wait = 0 : dv_office = 0
        END IF
        RETURN
    END IF
    dv_sample = mp
    GOSUB snd_sample
END

' The wampus: tu.wampus lists when (turn jiffies) and where it shows.
dv_wampus: PROCEDURE
    n = st8(ST_TURN_N_WAMPUS)
    #w = #dv_el + (FRAME - #rx_jiffy)          ' turn time now
    dv_wp = 255
    FOR k = 0 TO n - 1
        IF k >= 12 THEN EXIT FOR
        #addr = ST_TURN_WAMPUS + k * SZ_WAMPUS
        IF #w >= st16(#addr + S_WAMPUS_FROM) AND #w < st16(#addr + S_WAMPUS_TO) THEN dv_wp = k
    NEXT k
    IF dv_wp > 11 THEN
        IF wp_on THEN SPRITE MOB_WAMPUS, 0, 0, 0 : wp_on = 0
        RETURN
    END IF
    #addr = ST_TURN_WAMPUS + dv_wp * SZ_WAMPUS
    mp = st8(#addr + S_WAMPUS_PLOT)
    c = st8(#addr + S_WAMPUS_MOUNT)
    ' On the plot's mountains (top-left card): x + 1 + 2c, y + 1 + 2c.
    wp_x = (mp % 9) * 16 + 16 + 1 + c * 2
    wp_y = (mp / 9) * 16 + 16 + 1 + c * 2
    ' It flashes: 8 frames on, 8 off.
    IF FRAME AND 8 THEN
        SPRITE MOB_WAMPUS, wp_x + VISIBLE, wp_y + ZOOMY2, gram_word(G_WAMPUS, C_WHITE)
        wp_on = 1
    ELSE
        SPRITE MOB_WAMPUS, 0, 0, 0
    END IF
END

' ---------------------------------------------------------------------------
' The town screen (foreground/background mode, black street).
' ---------------------------------------------------------------------------
town_enter: PROCEDURE
    anim = AN_NONE
    GOSUB hide_mobs
    scr_bg = C_BLACK : GOSUB screen_fgbg
    g_slot = 0 : g_n = TOWN_NCARDS : #g_src = VARPTR gfx_town(0) : GOSUB gram_load
    g_slot = G_BAR : g_n = 8 : #g_src = VARPTR gfx_bar(0) : GOSUB gram_load
    g_slot = G_MULE : g_n = 2 : #g_src = VARPTR gfx_mule(0) : GOSUB gram_load
    scr_now = SCR_TOWN
    FOR y = 0 TO 8
        FOR x = 0 TO 11
            #BACKTAB((y + 1) * 20 + 4 + x) = town_screen(y * 12 + x)
        NEXT x
    NEXT y
    ' Title and money.
    #val = round : title_id = T_DEVELOPMENT
    t_pos = ROW_TITLE : GOSUB clear_row
    #lit_addr = VARPTR t_titles(0) : lit_n = title_id : GOSUB lit_nth
    #t_src = #lit_addr : GOSUB text_expand
    t_pos = ROW_TITLE + (20 - t_len) / 2 : #t_col = C_WHITE + fgbg_bg(C_BLACK)
    #t_src = TXT : GOSUB put_ram
    GOSUB town_money
    bar_col = 17 : bar_rows = 9 : bar_last = 255
    w_town = 1
    GOSUB dv_resume
END

' "MONEY EQUALS $n" under the town, and the corral's M.U.L.E. count/price.
town_money: PROCEDURE
    IF scr_now <> SCR_TOWN THEN RETURN
    msg_row = 200 : #msg_col = C_WHITE + fgbg_bg(C_BLACK)
    #val = pl16(my_seat, S_PLAYER_MONEY)
    msg_id = M_MONEY : GOSUB msg_show
    ' The corral (grey): price and how many are left.
    #t_col = C_BLACK + fgbg_bg(C_GREY)
    #val = st16(ST_STORE_MULE_PRICE)
    t_pos = 9 * 20 + 12 : t_len = 3 : GOSUB put_numr
    #val = st8(ST_STORE_MULES)
    t_pos = 6 * 20 + 13 : t_len = 2 : GOSUB put_numr
END
