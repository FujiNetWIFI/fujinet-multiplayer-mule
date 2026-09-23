' ===========================================================================
' grant.bas -- the land grant.
'
' The cursor sweeps the map a plot every CTU (12 jiffies), skipping the store
' and owned plots without losing time; the button claims the plot under it.
' The server judges the claim as of the press: it accepts a plot the cursor
' left up to 36 jiffies ago (our cursor runs late, and the claim waits for the
' poll in progress), and takes the plot from a computer that claimed it on
' arrival within that time. isr.bas moves the cursor between polls.
'
' Round 1 opens with the transport ship leaving ("the ship will be back in 12
' months") before the first step, as on the Atari; the server holds the
' cursor back for it (LandingDelay).
' ===========================================================================

    DIM gr_depart               ' 1 while the ship is leaving
    DIM gr_claimed              ' 1 once we've claimed this round
    CONST GR_WANT = $8812       ' the plot we claimed and are waiting on (255
                                ' none), in cart RAM (no 8-bit variables left)

grant_enter: PROCEDURE
    map_dots = 0
    GOSUB map_enter
    anim = AN_GRANT
    g_show = 0 : g_cur = 255 : g_next = 0
    gr_claimed = 0 : gr_depart = 0
    POKE GR_PRESS, 255 : POKE GR_WANT, 255 : POKE GR_WAS, 255
    msg_row = ROW_MSG : #msg_col = C_TAN
    IF round = 1 AND pe_now < 700 THEN
        ' The ship leaves the store first.
        gr_depart = 1
        title_id = T_TRANSPORT : GOSUB title_show
        msg_id = M_DEPART : GOSUB msg_show
        k = 0 : GOSUB fx_load_ship
        fa = 0 : #addr = VARPTR fx_depart(0) : GOSUB fx_start
    ELSE
        title_id = T_LANDGRANT : GOSUB title_show
        msg_id = M_SELECT : GOSUB msg_show
    END IF
END

grant_state: PROCEDURE
    GOSUB map_update
    ' Sync the cursor: -1 (255) before it starts, 45 when it's done.
    k = st8(ST_GRANT_CURSOR)
    POKE GR_SRV, k
    g_step = st8(ST_GRANT_STEP)
    IF g_step = 0 THEN g_step = 12
    #w = st16(ST_GRANT_NEXT)
    IF #w > 255 THEN #w = 255
    IF k <> g_cur OR #w < g_next THEN
        IF k <> g_cur THEN POKE GR_WAS, 255
        g_cur = k
        g_next = #w
        IF g_next = 0 THEN g_next = 1
    END IF
    IF gr_depart = 0 THEN g_show = 1
    IF my_seat < 4 THEN
        IF st8(ST_GRANT_CLAIMED + my_seat) < 45 THEN gr_claimed = 1 : POKE GR_WANT, 255
        j = PEEK(GR_WANT) AND 255
        IF j < 45 AND st8(ST_GRANT_CLAIMED + my_seat) > 44 THEN
            ' Every claim is answered by the next state, so a plot owned by a
            ' computer is not lost yet: the server may still give it to us.
            IF act_sent = ACT_CLAIM AND st8(ST_ERR) <> 0 AND k < j THEN
                ' Turned down because the server's cursor wasn't there yet
                ' (it holds after a computer's claim, which our cursor can't
                ' know): keep the claim, and send it again when it arrives.
                act_sent = 0
            ELSEIF k >= 45 OR k > j OR (act_sent = ACT_CLAIM AND st8(ST_ERR) <> 0) THEN
                ' Turned down, or the cursor is past it: press again.
                POKE GR_WANT, 255 : gr_claimed = 0 : GOSUB snd_error
            ELSEIF k = j AND act = ACT_NONE AND act_sent <> ACT_CLAIM THEN
                act = ACT_CLAIM : #act_arg = j : poll_wait = 0
            END IF
        END IF
    END IF
END

grant_tick: PROCEDURE
    GOSUB msg_tick_run
    IF gr_depart THEN
        IF fx_flag = 1 OR PEEK(FXB + AF_ST) = 0 THEN
            ' The ship has gone: the grant proper begins.
            gr_depart = 0
            GOSUB fx_stop_all
            g_slot = G_WAMPUS : g_n = 2 : #g_src = VARPTR gfx_cursor(0) : GOSUB gram_load
            title_id = T_LANDGRANT : GOSUB title_show
            msg_row = ROW_MSG : #msg_col = C_TAN : msg_id = M_SELECT : GOSUB msg_show
            g_show = 1
        END IF
        RETURN
    END IF
    IF ev_btn THEN
        ' Claim the plot the cursor was on when the button went down (see
        ' GR_PRESS), and hold the cursor there for the server's pause after
        ' a claim (4 steps); the next state corrects it if we lost.
        k = PEEK(GR_PRESS) AND 255
        IF k > 44 THEN k = g_cur
        POKE GR_PRESS, 255
        IF gr_claimed = 0 AND my_seat < 4 AND k < 45 THEN
            act = ACT_CLAIM : #act_arg = k : poll_wait = 0
            gr_claimed = 1 : POKE GR_WANT, k
            g_cur = k : g_next = g_step * 4 : POKE GR_WAS, 255
            GOSUB snd_buy
        END IF
    END IF
END
