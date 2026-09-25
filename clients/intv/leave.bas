' ===========================================================================
' leave.bas -- leaving the table: keypad CLEAR in the lobby at once; in a
' game after asking (CLEAR again leaves, anything else carries on). The
' server gives our seat to the computer and the table list comes up.
' ===========================================================================

    CONST LV_GO = $8820         ' 1: leave at the top of the next main loop
    CONST LV_ANIM = $8821       ' anim while we ask
    CONST LV_ROW = $8822        ' msg_row while we ask

' leave_ask: CLEAR in a game. The game stands still while we ask: anim off,
' so the handler neither walks nor takes the button.
leave_ask: PROCEDURE
    POKE LV_ANIM, anim : anim = AN_NONE
    POKE LV_ROW, msg_row
    msg_row = ROW_MSG : #msg_col = C_WHITE : msg_id = M_LEAVEASK : GOSUB msg_show
    DO
        WAIT
        GOSUB msg_tick_run
        GOSUB take_input
    LOOP WHILE ev_btn = 0 AND ev_key = 12
    IF ev_key = 10 THEN POKE LV_GO, 1 : RETURN
    GOSUB msg_clear
    anim = PEEK(LV_ANIM) AND 255
    msg_row = PEEK(LV_ROW) AND 255
    ev_btn = 0 : ev_key = 12
END

' leave_table: from the main loop itself (the table list goes deep).
leave_table: PROCEDURE
    GOSUB fx_stop_all
    anim = AN_NONE
    GOSUB hide_mobs
    act = ACT_LEAVE : GOSUB net_exchange
    GOSUB clear_room_appkey
    GOSUB table_select
    GOSUB write_room_appkey
    last_phase = 255 : scr_now = SCR_NONE : st_ok = 0 : poll_wait = 0
END
