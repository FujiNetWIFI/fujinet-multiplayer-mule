' ===========================================================================
' status.bas -- the status summary, the game-over screen and the lobby.
'
' Summary (410 jiffies of drawing, then everyone presses their button): the
' players walk in from the right along the bottom, 4 px every 6 frames, then
' climb to their rows in rank order; then the table prints.
'
' Table (black, colour stack):
'   row 0  STATUS SUMMARY #n
'   row 1        MONEY  LAND GOODS
'   rows 2..9  two rows a player, by rank: money land goods / total
'   row 10 COLONY total
'   row 11 press all player buttons to go on (scrolling)
' ===========================================================================

    CONST G_SU_WALK = 0         ' 16 cards: each player's walking-left frames x2
    CONST G_SU_CLIMB = 32       ' 16 cards: each player's climbing frames x2

    DIM su_order(4)             ' seats by rank
    DIM su_drawn, su_pressed

' The seats in rank order (rank 1 first).
su_rank: PROCEDURE
    n = 0
    FOR k = 1 TO 4
        FOR i = 0 TO 3
            IF pl8(i, S_PLAYER_RANK) = k AND n < 4 THEN su_order(n) = i : n = n + 1
        NEXT i
    NEXT k
    ' Anyone without a rank goes last.
    FOR i = 0 TO 3
        k = 0
        FOR j = 0 TO n - 1
            IF n > 0 THEN IF su_order(j) = i THEN k = 1
        NEXT j
        IF k = 0 AND n < 4 THEN su_order(n) = i : n = n + 1
    NEXT i
END

    DIM su_i, su_land

' Round 0's summary follows the colonists' arrival: the Atari lands the
' transport ship first ("you're landing on the planet Irata."), then draws
' the summary.
summary_enter: PROCEDURE
    anim = AN_NONE
    GOSUB fx_stop_all
    su_land = 0
    IF round = 0 AND pe_now < 300 THEN
        su_land = 1
        GOSUB map_enter
        title_id = T_TRANSPORT : GOSUB title_show
        msg_row = ROW_MSG : #msg_col = C_TAN : msg_id = M_LANDING : GOSUB msg_show
        k = 0 : GOSUB fx_load_ship
        fx_flag = 0
        fa = 0 : #addr = VARPTR fx_land(0) : GOSUB fx_start
        RETURN
    END IF
    GOSUB su_begin
END

su_begin: PROCEDURE
    anim = AN_NONE
    GOSUB fx_stop_all
    GOSUB hide_mobs
    #ph_t0 = FRAME
    scr_bg = C_BLACK : GOSUB screen_stack
    scr_now = SCR_SUMMARY
    su_drawn = 0 : su_pressed = 0
    GOSUB su_rank
    ' Walking-left frames 2/3 and climbing (back) frames 8/9 for each player.
    FOR su_i = 0 TO 3
        k = su_order(su_i)
        j = pl8(k, S_PLAYER_SPECIES) AND 7
        g_slot = G_SU_WALK + su_i * 8 : g_n = 8 : #g_src = VARPTR gfx_species((j * 11 + 2) * 16) : GOSUB gram_load
        g_slot = G_SU_CLIMB + su_i * 8 : g_n = 8 : #g_src = VARPTR gfx_species((j * 11 + 8) * 16) : GOSUB gram_load
    NEXT su_i
    ' If we arrive late, skip straight to the table.
    IF pe_now > 400 AND round > 0 THEN GOSUB su_table : RETURN
    FOR su_i = 0 TO 3
        k = su_order(su_i)
        c = pl_mobcol(pl8(k, S_PLAYER_COLOR) AND 3)
        #w = (c AND 7) + (c AND 8) * $200
        ' Script built in RAM would need 16-bit RAM; instead each actor runs
        ' the same script and is placed/timed from here.
        fa = su_i
        POKE FXB + AF_X + fa, 151 + 64 : POKE FXB + AF_Y + fa, 84 + 32
        #fx_f(fa) = $0800 + (G_SU_WALK + su_i * 8) * 8 + #w
        POKE FXB + AF_D2 + fa, 4
        POKE FXB + AF_KIND + fa, FK_PAIR : POKE FXB + AF_MOB + fa, su_i * 2
        POKE FXB + AF_XA + fa, 0 : POKE FXB + AF_YA + fa, 0
        POKE FXB + AF_C1 + fa, c
        #addr = VARPTR su_walk_scripts(su_i * 16)
        GOSUB fx_start
    NEXT su_i
END

' Walk-in scripts: each player enters 20 px (5 steps) after the one before,
' walks left to x 19 (33 steps), then climbs to row 2+2i (MOB y 24 + 16i)
' from y 84, 2 px every 6 frames. summary_enter sets the look; CARD switches
' to the climbing frames (summary_tick puts the colour back).
su_walk_scripts:
    DATA FX_WAIT, 1, FX_MOVE, -4, 0, 6, 33, FX_CARD, $0800 + (G_SU_CLIMB + 0) * 8, FX_MOVE, 0, -2, 6, 30, FX_END, 0
    DATA FX_WAIT, 30, FX_MOVE, -4, 0, 6, 33, FX_CARD, $0800 + (G_SU_CLIMB + 8) * 8, FX_MOVE, 0, -2, 6, 22, FX_END, 0
    DATA FX_WAIT, 60, FX_MOVE, -4, 0, 6, 33, FX_CARD, $0800 + (G_SU_CLIMB + 16) * 8, FX_MOVE, 0, -2, 6, 14, FX_END, 0
    DATA FX_WAIT, 90, FX_MOVE, -4, 0, 6, 33, FX_CARD, $0800 + (G_SU_CLIMB + 24) * 8, FX_MOVE, 0, -2, 6, 6, FX_END, 0

summary_state: PROCEDURE
    IF su_land THEN RETURN
    IF su_drawn = 0 AND FRAME - #ph_t0 >= 410 THEN GOSUB su_table
    IF su_drawn THEN GOSUB su_values
END

summary_tick: PROCEDURE
    IF su_land THEN
        GOSUB msg_tick_run
        IF fx_flag = 1 OR PEEK(FXB + AF_ST) = 0 THEN su_land = 0 : GOSUB su_begin
        RETURN
    END IF
    ' The FX_CARD above loses the player's colour: put it back once climbing.
    FOR su_i = 0 TO 3
        IF PEEK(FXB + AF_ST + su_i) AND 255 THEN
            c = PEEK(FXB + AF_C1 + su_i) AND 255
            IF (#fx_f(su_i) AND $1007) = 0 AND c THEN
                #fx_f(su_i) = #fx_f(su_i) + (c AND 7) + (c AND 8) * $200
            END IF
        END IF
    NEXT su_i
    IF su_drawn = 0 AND FRAME - #ph_t0 >= 410 THEN GOSUB su_table
    IF su_drawn THEN
        GOSUB msg_tick_run
        IF ev_btn AND su_pressed = 0 AND my_seat < 4 THEN
            su_pressed = 1 : act = ACT_READY : poll_wait = 0 : GOSUB snd_select
        END IF
    END IF
END

' The table.
su_table: PROCEDURE
    su_drawn = 1
    GOSUB fx_stop_all
    ' Players stand at the left of their rows.
    FOR su_i = 0 TO 3
        k = su_order(su_i)
        c = pl_mobcol(pl8(k, S_PLAYER_COLOR) AND 3)
        #w = $0800 + (G_SU_CLIMB + su_i * 8) * 8 + (c AND 7) + (c AND 8) * $200
        y = 24 + su_i * 16
        SPRITE su_i * 2, 10 + VISIBLE, y + DOUBLEY, #w
        SPRITE su_i * 2 + 1, 10 + VISIBLE, (y + 8) + DOUBLEY, #w + 16
    NEXT su_i
    ' Title.
    #lit_addr = VARPTR t_words(0) : lit_n = 0 : GOSUB lit_nth
    #t_src = #lit_addr : GOSUB text_expand
    POKE TXT + t_len, 48 + round / 10 : IF round >= 10 THEN t_len = t_len + 1
    POKE TXT + t_len, 48 + round % 10 : t_len = t_len + 1
    POKE TXT + t_len, 0
    t_pos = (20 - t_len) / 2 : #t_col = C_YELLOW : t_len = 20 : #t_src = TXT : GOSUB put_ram
    ' Column heads.
    #t_col = C_WHITE
    #lit_addr = VARPTR t_words(0) : lit_n = 1 : GOSUB lit_nth : #t_src = #lit_addr : t_pos = 20 + 4 : GOSUB put_lstr
    #lit_addr = VARPTR t_words(0) : lit_n = 2 : GOSUB lit_nth : #t_src = #lit_addr : t_pos = 20 + 10 : GOSUB put_lstr
    #lit_addr = VARPTR t_words(0) : lit_n = 3 : GOSUB lit_nth : #t_src = #lit_addr : t_pos = 20 + 15 : GOSUB put_lstr
    #lit_addr = VARPTR t_words(0) : lit_n = 5 : GOSUB lit_nth : #t_src = #lit_addr : t_pos = 200 + 3 : #t_col = C_YELLOW : GOSUB put_lstr
    GOSUB su_values
    IF phase = PHASE_SUMMARY THEN
        msg_row = ROW_MSG : #msg_col = C_WHITE : msg_id = M_PRESSALL : GOSUB msg_show
    END IF
END

' The numbers (they change as players press, so redrawn per state).
su_values: PROCEDURE
    FOR su_i = 0 TO 3
        k = su_order(su_i)
        #t_col = pl_txtcol(pl8(k, S_PLAYER_COLOR) AND 3)
        ' Money, land, goods: the server's ScoreOf, worked out from the map
        ' and the store's prices (the binary sends only money and the total).
        #val = pl16(k, S_PLAYER_MONEY)
        t_pos = (2 + su_i * 2) * 20 + 3 : t_len = 6 : GOSUB put_numr
        GOSUB su_land
        t_pos = (2 + su_i * 2) * 20 + 9 : t_len = 5 : GOSUB put_numr
        #val = 0
        FOR j = 0 TO 3
            #val = #val + pl8(k, S_PLAYER_GOODS + j) * st16(ST_STORE_PRICE + j * 2)
        NEXT j
        t_pos = (2 + su_i * 2) * 20 + 14 : t_len = 6 : GOSUB put_numr
        ' second row: the total
        #lit_addr = VARPTR t_words(0) : lit_n = 4 : GOSUB lit_nth : #t_src = #lit_addr
        t_pos = (3 + su_i * 2) * 20 + 3 : GOSUB put_lstr
        #val = pl16(k, S_PLAYER_SCORE)
        t_pos = (3 + su_i * 2) * 20 + 13 : t_len = 7 : GOSUB put_numr
    NEXT su_i
    ' Colony total (32-bit in the binary).
    #val = st16(ST_COLONY_TOTAL) : #val2 = st16(ST_COLONY_TOTAL + 2)
    t_pos = 200 + 13 : t_len = 7 : #t_col = C_YELLOW : GOSUB put_num32
END

' Land value of seat k: 500 a plot, plus the outfit cost + 35 for each
' M.U.L.E. on one (the server's ScoreOf) -> #val.
su_land: PROCEDURE
    #val = 0
    FOR mp = 0 TO 44
        IF st8(ST_MAP_OWNER + mp) = k THEN
            #val = #val + 500
            mg = st8(ST_MAP_MULE + mp)
            IF mg < 4 THEN #val = #val + (mg + 1) * 25 + 35
        END IF
    NEXT mp
END

' ---------------------------------------------------------------------------
' Game over: FIRST FOUNDER, then the final summary and the colony's rating.
' ---------------------------------------------------------------------------
    DIM go_stage
gameover_enter: PROCEDURE
    anim = AN_NONE
    GOSUB fx_stop_all
    GOSUB hide_mobs
    scr_bg = C_BLACK : GOSUB screen_stack
    scr_now = SCR_FOUNDER
    go_stage = 0
    GOSUB su_rank
    FOR su_i = 0 TO 3
        k = su_order(su_i)
        j = pl8(k, S_PLAYER_SPECIES) AND 7
        g_slot = G_SU_WALK + su_i * 8 : g_n = 4 : #g_src = VARPTR gfx_species((j * 11 + 5) * 16) : GOSUB gram_load
    NEXT su_i
    ' "FIRST <winner> FOUNDER": the words either side of the winner.
    #lit_addr = VARPTR t_words(0) : lit_n = 6 : GOSUB lit_nth
    #t_src = #lit_addr + 1 : t_pos = 20 + 3 : t_len = 5 : #t_col = C_GREEN : GOSUB put_text
    #t_src = #lit_addr + 7 : t_pos = 20 + 10 : t_len = 7 : GOSUB put_text
    ' The others stand down the left.
    FOR su_i = 1 TO 3
        k = su_order(su_i)
        c = pl_mobcol(pl8(k, S_PLAYER_COLOR) AND 3)
        #w = $0800 + (G_SU_WALK + su_i * 8) * 8 + (c AND 7) + (c AND 8) * $200
        y = 32 + su_i * 18
        SPRITE su_i * 2, 12 + VISIBLE, y + DOUBLEY, #w
        SPRITE su_i * 2 + 1, 12 + VISIBLE, (y + 8) + DOUBLEY, #w + 16
    NEXT su_i
    GOSUB snd_founder
END

gameover_tick: PROCEDURE
    IF go_stage = 0 THEN
        ' The winner pulses through the luminances of its colour, ~32 frames
        ' a step, for about 480 frames.
        k = su_order(0)
        c = pl_mobcol(pl8(k, S_PLAYER_COLOR) AND 3)
        IF (FRAME AND 32) THEN c = C_WHITE
        #w = $0800 + G_SU_WALK * 8 + (c AND 7) + (c AND 8) * $200
        SPRITE 0, 76 + VISIBLE, 14 + DOUBLEY, #w
        SPRITE 1, 76 + VISIBLE, 22 + DOUBLEY, #w + 16
        IF pe_now > 540 THEN
            go_stage = 1
            GOSUB hide_mobs
            FOR i = 0 TO 11 : t_pos = i * 20 : GOSUB clear_row : NEXT i
            ' The final summary, climbing frames standing in.
            FOR su_i = 0 TO 3
                k = su_order(su_i) : j = pl8(k, S_PLAYER_SPECIES) AND 7
                g_slot = G_SU_CLIMB + su_i * 8 : g_n = 4 : #g_src = VARPTR gfx_species((j * 11 + 5) * 16) : GOSUB gram_load
            NEXT su_i
            GOSUB su_table
            ' "*** OVERALL, " + the colony's rating, scrolling.
            #lit_addr = VARPTR t_ratings(0) : lit_n = st8(ST_COLONY_RATING) : GOSUB lit_nth
            IF lit_n > 6 THEN RETURN
            #t_src = #lit_addr : GOSUB text_expand
            msg_row = ROW_MSG : #msg_col = C_WHITE : GOSUB msg_start
        END IF
    ELSE
        GOSUB msg_tick_run
    END IF
END

' ---------------------------------------------------------------------------
' The lobby: seats, species, ready. (The Atari's "you have selected a
' tournament level game" screen.)
'   rows 0-1  M.U.L.E. / TOURNAMENT       rows 3..10 two rows a seat
'   row 11    countdown or help
' Disc left/right changes our species, the button toggles ready.
' ---------------------------------------------------------------------------
    DIM lb_species, lb_sent, lb_last(4)
    DIM #lb_lastcd
lobby_enter: PROCEDURE
    anim = AN_NONE
    GOSUB fx_stop_all
    GOSUB hide_mobs
    scr_bg = C_BLACK : GOSUB screen_stack
    scr_now = SCR_TEXT
    FOR k = 0 TO 3 : lb_last(k) = 255 : NEXT k
    #lit_addr = VARPTR t_words(0) : lit_n = 13 : GOSUB lit_nth : #t_src = #lit_addr
    t_pos = 0 : #t_col = C_YELLOW : GOSUB put_lstr_c
    #lit_addr = VARPTR t_words(0) : lit_n = 14 : GOSUB lit_nth : #t_src = #lit_addr
    t_pos = 20 : #t_col = C_BLUE : GOSUB put_lstr_c
    lb_sent = 0
    IF my_seat < 4 THEN lb_species = pl8(my_seat, S_PLAYER_SPECIES) AND 7
END

lobby_state: PROCEDURE
    FOR k = 0 TO 3
        #addr = (3 + k * 2) * 20
        c = pl8(k, S_PLAYER_FLAGS)
        j = pl8(k, S_PLAYER_SPECIES) AND 7
        IF lb_last(k) <> j + c * 8 THEN
            lb_last(k) = j + c * 8
            t_pos = #addr : GOSUB clear_row
            t_pos = #addr + 20 : GOSUB clear_row
            #t_col = pl_txtcol(pl8(k, S_PLAYER_COLOR) AND 3)
            IF PEEK(STATE + ST_PLAYERS + k * SZ_PLAYER) AND 255 THEN
                #t_src = STATE + ST_PLAYERS + k * SZ_PLAYER : t_pos = #addr + 3 : t_len = 8 : GOSUB put_ram
                #lit_addr = VARPTR t_species(0) : lit_n = j : GOSUB lit_nth : #t_src = #lit_addr
                t_pos = #addr + 12 : GOSUB put_lstr
                IF c AND PF_AI THEN
                    #lit_addr = VARPTR t_words(0) : lit_n = 17 : GOSUB lit_nth
                ELSEIF c AND PF_READY THEN
                    #lit_addr = VARPTR t_msgs(0) : lit_n = M_READY : GOSUB lit_nth
                ELSE
                    #lit_addr = 0
                END IF
                IF #lit_addr THEN #t_src = #lit_addr : t_pos = #addr + 23 : #t_col = C_WHITE : GOSUB put_lstr
                ' The species, standing.
                g_slot = k * 4 : g_n = 4 : #g_src = VARPTR gfx_species((j * 11 + 5) * 16) : GOSUB gram_load
                c = pl_mobcol(pl8(k, S_PLAYER_COLOR) AND 3)
                #w = $0800 + k * 32 + (c AND 7) + (c AND 8) * $200
                y = (3 + k * 2) * 8 + 8
                SPRITE k * 2, 12 + VISIBLE, y + DOUBLEY, #w
                SPRITE k * 2 + 1, 12 + VISIBLE, (y + 8) + DOUBLEY, #w + 16
            ELSE
                #lit_addr = VARPTR t_words(0) : lit_n = 18 : GOSUB lit_nth : #t_src = #lit_addr
                t_pos = #addr + 3 : #t_col = C_WHITE : GOSUB put_lstr
                SPRITE k * 2, 0, 0, 0 : SPRITE k * 2 + 1, 0, 0, 0
            END IF
        END IF
    NEXT k
    IF my_seat > 3 THEN
        msg_row = ROW_MSG : #msg_col = C_WHITE : msg_id = M_SPECTATE : GOSUB msg_show
    END IF
END

lobby_tick: PROCEDURE
    GOSUB msg_tick_run
    ' Keypad CLEAR: leave this table and pick another.
    IF ev_key = 10 THEN POKE LV_GO, 1 : RETURN   ' the main loop leaves
    ' Countdown to the start, in seconds.
    #w = st16(ST_COUNTDOWN)
    IF #w < 32768 THEN
        #val = #w - (FRAME - #rx_jiffy)
        IF #val > 32768 THEN #val = 0
        #val = #val / 60
        IF #val <> #lb_lastcd THEN
            #lb_lastcd = #val
            t_pos = ROW_MSG : GOSUB clear_row
            t_pos = ROW_MSG + 9 : t_len = 2 : #t_col = C_YELLOW : GOSUB put_numr
        END IF
    ELSEIF #lb_lastcd <> 65535 THEN
        #lb_lastcd = 65535
        msg_row = ROW_MSG : #msg_col = C_WHITE : msg_id = M_WAITING : GOSUB msg_show
    END IF
    IF my_seat > 3 THEN RETURN
    ' Species: disc left/right, one change per push.
    IF inp_dir AND 10 THEN
        IF lb_sent = 0 THEN
            lb_sent = 1
            IF inp_dir AND 2 THEN lb_species = (lb_species + 1) AND 7 ELSE lb_species = (lb_species + 7) AND 7
            act = ACT_SPECIES : #act_arg = lb_species : poll_wait = 0
            GOSUB snd_select
        END IF
    ELSE
        lb_sent = 0
    END IF
    IF ev_btn THEN
        IF pl8(my_seat, S_PLAYER_FLAGS) AND PF_READY THEN act = ACT_UNREADY ELSE act = ACT_READY
        poll_wait = 0
        GOSUB snd_select
    END IF
END
