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
' (segment 0: segment 1 is full)
    SEGMENT 0
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
    SEGMENT 1

' ---------------------------------------------------------------------------
' Spectating: a seatless player (the game had started when we joined, or we
' left it) can only leave -- with CLEAR, at once, as there is no seat to
' lose -- and every other input is dropped. Row 0 has no room beside a
' title, so for a second in every two the tag takes the whole title row,
' in the title's colour; the row is put back after, unless the screen has
' redrawn it meanwhile.
' ---------------------------------------------------------------------------
    CONST SP_ON = $8823         ' 1: the tag is up
    CONST SP_WORD = $8824       ' 2 bytes: the card the tag put at column 5
    CONST SP_SAVE = $8830       ' row 0 under the tag: 20 words as 40 bytes

spec_txt:
    DATA 9, "SPECTATOR"

' spec_tick: from the main loop, every frame once a state is in.
spec_tick: PROCEDURE
    IF my_seat > 3 THEN
        ev_btn = 0
        IF ev_key = 10 AND phase > 0 THEN POKE LV_GO, 1 : ev_key = 12
        IF ev_key <> 10 THEN ev_key = 12
        IF FRAME AND 64 THEN
            IF PEEK(SP_ON) AND 255 THEN RETURN
            IF scr_now = SCR_NONE THEN RETURN
            #h_w = 0
            FOR h_i = 0 TO 19
                #h_a = #BACKTAB(h_i)
                POKE SP_SAVE + h_i * 2, #h_a AND 255
                POKE SP_SAVE + h_i * 2 + 1, #h_a / 256
                IF #h_w = 0 AND (#h_a AND $09F8) <> 0 THEN #h_w = #h_a OR $8000
            NEXT h_i
            IF scr_mode = 1 THEN
                IF #h_w THEN #t_col = #h_w AND $3607 ELSE #t_col = C_WHITE + fgbg_bg(scr_bg)
            ELSE
                IF #h_w THEN #t_col = #h_w AND $1007 ELSE #t_col = C_WHITE
            END IF
            t_pos = 0 : GOSUB clear_row
            t_pos = 5 : #t_src = VARPTR spec_txt(0) : GOSUB put_lstr
            #h_a = #BACKTAB(5)
            POKE SP_WORD, #h_a AND 255 : POKE SP_WORD + 1, #h_a / 256
            POKE SP_ON, 1
            RETURN
        END IF
    END IF
    IF (PEEK(SP_ON) AND 255) = 0 THEN RETURN
    POKE SP_ON, 0
    IF #BACKTAB(5) <> (PEEK(SP_WORD) AND 255) + (PEEK(SP_WORD + 1) AND 255) * 256 THEN RETURN
    FOR h_i = 0 TO 19
        #BACKTAB(h_i) = (PEEK(SP_SAVE + h_i * 2) AND 255) + (PEEK(SP_SAVE + h_i * 2 + 1) AND 255) * 256
    NEXT h_i
END
