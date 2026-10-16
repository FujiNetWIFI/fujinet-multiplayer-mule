' ===========================================================================
' grant.bas -- the land grant.
'
' The cursor sweeps the map a plot every CTU (12 jiffies), skipping the store
' and owned plots without losing time; the button claims the plot under it.
' The server judges the claim as of the press: it accepts a plot the cursor
' left up to 36 jiffies ago, and takes the plot from a computer that claimed
' it on arrival within that time.
'
' Between polls isr_grant steps the cursor on its own, from the state's
' countdown less the state's age, never past the plot the state says a
' computer will stop it on (an older server doesn't say: then at most one
' step past the server's cursor).
'
' Round 1 opens with the transport ship leaving ("the ship will be back in 12
' months") before the first step, as on the Atari; the server holds the
' cursor back for it (LandingDelay).
' ===========================================================================

    DIM gr_depart               ' 1 while the ship is leaving
    DIM gr_claimed              ' 1 once we've claimed this round

grant_enter: PROCEDURE
    map_dots = 0
    GOSUB map_enter
    anim = AN_GRANT
    g_show = 0 : g_cur = 255 : g_next = 0
    gr_claimed = 0 : gr_depart = 0
    POKE GR_PRESS, 255 : POKE GR_WANT, 255 : POKE GR_WAS, 255 : POKE GR_LIM, 255 : POKE GR_PARK, 0
    POKE GR_LAG, PEEK(STATE + ST_T) - FRAME
    msg_row = ROW_MSG : #msg_col = C_TAN
    IF round = 1 AND pe_now < 700 THEN
        ' The ship leaves the store first.
        gr_depart = 1
        title_id = T_TRANSPORT : GOSUB title_show
        msg_id = M_DEPART : GOSUB msg_show
        k = 0 : GOSUB fx_load_ship
        fx_flag = 0             ' (the summary's landing left it 1)
        fa = 0 : #addr = VARPTR fx_depart(0) : GOSUB fx_start
    ELSE
        title_id = T_LANDGRANT : GOSUB title_show
        msg_id = M_SELECT : GOSUB msg_show
        GOSUB snd_alert
    END IF
END

' The ship has gone: the grant proper begins. (segment 3: segment 1 is full)
    SEGMENT 3
depart_done: PROCEDURE
    gr_depart = 0
    GOSUB fx_stop_all
    g_slot = G_WAMPUS : g_n = 2 : #g_src = VARPTR gfx_cursor(0) : GOSUB gram_load
    title_id = T_LANDGRANT : GOSUB title_show
    msg_row = ROW_MSG : #msg_col = C_TAN : msg_id = M_SELECT : GOSUB msg_show
    GOSUB snd_alert
    g_show = 1
END
    SEGMENT 1

grant_state: PROCEDURE
    GOSUB map_update
    ' The server's cursor: -1 (255) before it starts, 45 when it's done.
    k = st8(ST_GRANT_CURSOR)
    g_step = st8(ST_GRANT_STEP)
    ' The state's age: how much later than the fastest reply yet it is (the
    ' server's clock against FRAME, which never misses a frame).
    n = PEEK(STATE + ST_T) - FRAME
    j = n - PEEK(GR_LAG)
    IF j < 128 THEN POKE GR_LAG, n : j = 0 ELSE j = 256 - j
    ' Its countdown less the age: a step that came since is due now. Far
    ' off (the ship leaving), wait for a nearer state.
    #w = st16(ST_GRANT_NEXT)
    IF #w > j THEN #w = #w - j ELSE #w = 1
    IF #w > 255 THEN #w = 0
    IF my_seat < 4 THEN
        c = st8(ST_GRANT_CLAIMED + my_seat)
        IF c < 45 THEN gr_claimed = 1 : POKE GR_WANT, 255
        j = PEEK(GR_WANT) AND 255
        IF j < 45 THEN
            IF c > 44 THEN
                ' Only the state answering the claim judges it: those before
                ' were made before the server saw it (its cursor may have
                ' gone on, and a computer's plot may still be ours).
                i = 0
                IF act_sent = ACT_CLAIM THEN
                    ' Turned down as the server's cursor wasn't there yet:
                    ' send it again when it is. Otherwise: press again.
                    IF st8(ST_ERR) <> 0 AND (k < j OR k = 255) THEN POKE GR_PARK, 1 ELSE i = 1
                ELSEIF PEEK(GR_PARK) AND 1 THEN
                    IF act = ACT_NONE AND k >= j AND k <> 255 THEN
                        POKE GR_PARK, 0
                        IF k < 45 THEN act = ACT_CLAIM : #act_arg = j : poll_wait = 0 ELSE i = 1
                    END IF
                END IF
                IF i THEN POKE GR_WANT, 255 : POKE GR_PARK, 0 : gr_claimed = 0 : GOSUB snd_error
            END IF
        END IF
    END IF
    ' (while our claim is out the cursor waits on its plot)
    IF PEEK(GR_WANT) AND 128 THEN
        IF k = g_cur THEN
            g_next = #w
        ELSEIF (k < g_cur OR k = 255) AND g_cur < 45 AND #w <= g_step THEN
            ' We made the step it waits for a little early: no jump back
            ' (after a claim it waits longer, and the cursor goes there).
            g_next = #w + g_step
        ELSE
            g_next = #w : g_cur = k : POKE GR_WAS, 255
        END IF
    END IF
    ' No further than where a computer will stop it (an older server
    ' doesn't say: one plot past its cursor).
    j = st8(ST_GRANT_STOP)
    IF j THEN POKE GR_LIM, j - 1 ELSE POKE GR_LIM, k + 1
    ' The server's cursor has started: our ship (which set out late, after
    ' the map) has gone too, or the first plots would pass unseen.
    IF gr_depart THEN IF k < 45 THEN GOSUB depart_done
    IF gr_depart = 0 THEN g_show = 1
END

grant_tick: PROCEDURE
    GOSUB msg_tick_run
    IF gr_depart THEN
        IF fx_flag = 1 OR PEEK(FXB + AF_ST) = 0 THEN GOSUB depart_done
        RETURN
    END IF
    IF ev_btn THEN
        ' Claim the plot the cursor was on when the button went down (see
        ' GR_PRESS), and hold the cursor there until the server answers.
        k = PEEK(GR_PRESS) AND 255
        IF k > 44 THEN k = g_cur
        POKE GR_PRESS, 255
        IF gr_claimed = 0 AND my_seat < 4 AND k < 45 THEN
            act = ACT_CLAIM : #act_arg = k : poll_wait = 0
            gr_claimed = 1 : POKE GR_WANT, k
            g_cur = k : POKE GR_WAS, 255
            GOSUB snd_buy
        END IF
    END IF
END

