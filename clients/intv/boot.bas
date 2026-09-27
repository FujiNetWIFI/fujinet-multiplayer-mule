' ===========================================================================
' boot.bas -- start-up: FujiNet, who we are, which table.
'
' The FujiNet lobby hands a game its details in appkeys (creator 1, app 1):
'   key 0            the player's name (shared by every FujiNet game)
'   key AK_SERVER    the table's URL, "https://host/?table=<id>"
' Without them we ask for a name and list the default server's tables.
' (Pattern from fujinet-5cardstud/intv/5card.bas.)
' ===========================================================================

    CONST AK_SERVER = 10        ' M.U.L.E.'s lobby appkey

    DIM bt_try, bt_ok, bt_i, bt_c, bt_n, bt_sel, bt_top

boot_start:
    scr_bg = C_BLACK : GOSUB screen_stack
    scr_now = SCR_TEXT
    msg_row = ROW_MSG
    #lit_addr = VARPTR t_words(0) : lit_n = 13 : GOSUB lit_nth : #t_src = #lit_addr
    t_pos = 60 : #t_col = C_YELLOW : GOSUB put_lstr_c
    #lit_addr = VARPTR t_words(0) : lit_n = 14 : GOSUB lit_nth : #t_src = #lit_addr
    t_pos = 100 : #t_col = C_BLUE : GOSUB put_lstr_c
    PRINT AT 160 COLOR C_WHITE, "CONNECTING..."

    WHILE 1
        GOSUB fn_wait_mailbox
        IF fn_ok THEN EXIT WHILE
        PRINT AT 160 COLOR C_RED, "NO FUJINET FOUND    "
        FOR bt_i = 1 TO 60 : WAIT : NEXT bt_i
    WEND

    ' ---- the player's name ----
    bt_ok = 0
    FOR bt_try = 0 TO 1
        ak_creator_lo = 1 : ak_creator_hi = 0 : ak_app = 1 : ak_key = 0 : ak_mode = 0
        GOSUB appkey_open
        IF fn_ok THEN EXIT FOR
    NEXT bt_try
    IF fn_ok THEN
        #fn_src = SC_NAME : ls_max = 9 : GOSUB appkey_read
        GOSUB appkey_close
        IF fn_len > 0 THEN
            ' Only A-Z / 0-9 go into the URL unescaped.
            bt_ok = 1
            FOR bt_i = 0 TO fn_len - 1
                bt_c = PEEK(SC_NAME + bt_i) AND 255
                IF bt_c >= 97 AND bt_c <= 122 THEN bt_c = bt_c - 32
                IF (bt_c < 65 OR bt_c > 90) AND (bt_c < 48 OR bt_c > 57) THEN bt_ok = 0
            NEXT bt_i
        END IF
    END IF
    IF bt_ok = 0 THEN GOSUB name_entry

    ' ---- the server and table ----
    GOSUB endpoint_default
    POKE SC_TABLE, 0
    FOR bt_try = 0 TO 1
        ak_creator_lo = 1 : ak_creator_hi = 0 : ak_app = 1 : ak_key = AK_SERVER : ak_mode = 0
        GOSUB appkey_open
        IF fn_ok THEN EXIT FOR
    NEXT bt_try
    IF fn_ok THEN
        #fn_src = SC_ENDPT : ls_max = 65 : GOSUB appkey_read
        GOSUB appkey_close
        ' Only a URL counts: an unset key can read back as zeros.
        IF fn_len > 4 AND (PEEK(SC_ENDPT) AND 255) = 104 THEN
            GOSUB split_room_url
        ELSE
            GOSUB endpoint_default
        END IF
    END IF
    GOSUB endpoint_trim
    IF (PEEK(SC_TABLE) AND 255) = 0 THEN GOSUB table_select
    GOSUB write_room_appkey
    GOTO game_start

' ---------------------------------------------------------------------------
endpoint_default: PROCEDURE
    bt_n = lit_endpoint(0)
    FOR bt_i = 1 TO bt_n
        POKE SC_ENDPT + bt_i - 1, lit_endpoint(bt_i) + 32
    NEXT bt_i
    POKE SC_ENDPT + bt_n, 0
END

' Drop a trailing '/', so "<endpoint>/state" never has two.
endpoint_trim: PROCEDURE
    #fn_src = SC_ENDPT : ls_max = 64 : GOSUB fn_strlen
    IF fn_len > 0 THEN
        IF (PEEK(SC_ENDPT + fn_len - 1) AND 255) = 47 THEN POKE SC_ENDPT + fn_len - 1, 0
    END IF
END

' split_room_url: "https://host/?table=xyz" in SC_ENDPT (fn_len long) ->
' SC_ENDPT "https://host/", SC_TABLE "xyz" (A-Z a-z 0-9 only, else empty).
    DIM sp_found, sp_ok, sp_j, sp_k, sp_c
split_room_url: PROCEDURE
    POKE SC_TABLE, 0
    sp_found = 255
    FOR bt_i = 0 TO fn_len - 1
        IF (PEEK(SC_ENDPT + bt_i) AND 255) = 63 THEN sp_found = bt_i : EXIT FOR
    NEXT bt_i
    IF sp_found = 255 THEN RETURN
    POKE SC_ENDPT + sp_found, 0
    IF sp_found + 7 > fn_len THEN RETURN
    sp_ok = 1
    FOR bt_i = 0 TO 5
        IF (PEEK(SC_ENDPT + sp_found + 1 + bt_i) AND 255) <> lit_tableq(bt_i) + 32 THEN sp_ok = 0
    NEXT bt_i
    IF sp_ok = 0 THEN RETURN
    sp_j = sp_found + 7 : sp_k = 0
    WHILE sp_k < 8 AND sp_j < fn_len
        sp_c = PEEK(SC_ENDPT + sp_j) AND 255
        IF sp_c = 0 OR sp_c = 38 THEN EXIT WHILE
        IF (sp_c < 65 OR sp_c > 90) AND (sp_c < 97 OR sp_c > 122) AND (sp_c < 48 OR sp_c > 57) THEN POKE SC_TABLE, 0 : RETURN
        POKE SC_TABLE + sp_k, sp_c
        sp_k = sp_k + 1 : sp_j = sp_j + 1
    WEND
    POKE SC_TABLE + sp_k, 0
END
lit_tableq: DATA "table="

' Remember the table, so a reset goes straight back to it.
write_room_appkey: PROCEDURE
    #fn_txlen = 0
    #fn_src = SC_ENDPT : ls_max = 64 : GOSUB fn_strlen : GOSUB fn_putstr
    POKE FN_TX + #fn_txlen, 47 : #fn_txlen = #fn_txlen + 1           ' "/"
    #lit_addr = VARPTR lit_qtable(0) : GOSUB put_lit
    #fn_src = SC_TABLE : ls_max = 8 : GOSUB fn_strlen : GOSUB fn_putstr
    bt_n = #fn_txlen
    ak_creator_lo = 1 : ak_creator_hi = 0 : ak_app = 1 : ak_key = AK_SERVER : ak_mode = 1
    GOSUB appkey_open
    IF fn_ok THEN
        ' appkey_open staged its own 6 bytes in FN_TX: rebuild the value.
        #fn_txlen = 0
        #fn_src = SC_ENDPT : ls_max = 64 : GOSUB fn_strlen : GOSUB fn_putstr
        POKE FN_TX + #fn_txlen, 47 : #fn_txlen = #fn_txlen + 1
        #lit_addr = VARPTR lit_qtable(0) : GOSUB put_lit
        #fn_src = SC_TABLE : ls_max = 8 : GOSUB fn_strlen : GOSUB fn_putstr
        fn_len = #fn_txlen : #fn_src = FN_TX
        GOSUB appkey_write
        GOSUB appkey_close
    END IF
END

' Forget the table (after leaving it).
clear_room_appkey: PROCEDURE
    ak_creator_lo = 1 : ak_creator_hi = 0 : ak_app = 1 : ak_key = AK_SERVER : ak_mode = 1
    GOSUB appkey_open
    IF fn_ok THEN
        fn_len = 0 : #fn_src = FN_TX
        GOSUB appkey_write
        GOSUB appkey_close
    END IF
    POKE SC_TABLE, 0
END

' ---------------------------------------------------------------------------
' name_entry: pick letters with the disc (up/down), right for the next
' letter, left to rub out, the button when done. A-Z and 0-9, up to 8.
' ---------------------------------------------------------------------------
    DIM ne_len, ne_ch, ne_rep
name_entry: PROCEDURE
    scr_bg = C_BLACK : GOSUB screen_stack
    PRINT AT 40 COLOR C_YELLOW, "YOUR NAME?"
    PRINT AT 180 COLOR C_WHITE, "DISC: LETTER  BUTTON"
    PRINT AT 200 COLOR C_WHITE, "WHEN DONE"
    ne_len = 0 : ne_ch = 65
    FOR bt_i = 0 TO 8 : POKE SC_NAME + bt_i, 0 : NEXT bt_i
    WHILE 1
        WAIT
        GOSUB take_input
        IF ne_rep > 0 THEN ne_rep = ne_rep - 1
        IF ne_rep = 0 THEN
            IF inp_dir AND 4 THEN
                ne_ch = ne_ch + 1 : IF ne_ch = 91 THEN ne_ch = 48
                IF ne_ch = 58 THEN ne_ch = 65
                ne_rep = 8
            ELSEIF inp_dir AND 1 THEN
                ne_ch = ne_ch - 1 : IF ne_ch = 47 THEN ne_ch = 90
                IF ne_ch = 64 THEN ne_ch = 57
                ne_rep = 8
            ELSEIF inp_dir AND 2 THEN
                IF ne_len < 7 THEN POKE SC_NAME + ne_len, ne_ch : ne_len = ne_len + 1
                ne_rep = 15
            ELSEIF inp_dir AND 8 THEN
                IF ne_len > 0 THEN ne_len = ne_len - 1 : POKE SC_NAME + ne_len, 0
                ne_rep = 15
            END IF
        END IF
        IF ev_btn THEN
            POKE SC_NAME + ne_len, ne_ch : ne_len = ne_len + 1
            POKE SC_NAME + ne_len, 0
            EXIT WHILE
        END IF
        ' Show the name with the letter being chosen flashing after it.
        FOR bt_i = 0 TO 8
            IF bt_i < ne_len THEN
                #BACKTAB(85 + bt_i) = ((PEEK(SC_NAME + bt_i) AND 255) - 32) * 8 + C_WHITE
            ELSEIF bt_i = ne_len AND (FRAME AND 16) THEN
                #BACKTAB(85 + bt_i) = (ne_ch - 32) * 8 + C_YELLOW
            ELSE
                #BACKTAB(85 + bt_i) = 0
            END IF
        NEXT bt_i
    WEND
    GOSUB snd_select
END

' ---------------------------------------------------------------------------
' table_select: /tables?bin=1 is a count, then 33-byte entries of
' id[8], name[20], players[5] ("n / 4"). Disc up/down, button to join.
' ---------------------------------------------------------------------------
    CONST TB_STRIDE = 33
    CONST TB_LIST = $8000       ' STATE doubles as the list's buffer here
table_select: PROCEDURE
    WHILE 1
        scr_bg = C_BLACK : GOSUB screen_stack
        PRINT AT 0 COLOR C_YELLOW, "CHOOSE A TABLE"
        PRINT AT 220 COLOR C_WHITE, "LOADING..."
        #fn_txlen = 0
        POKE FN_TX, 78 : POKE FN_TX + 1, 58 : #fn_txlen = 2
        #fn_src = SC_ENDPT : ls_max = 64 : GOSUB fn_strlen : GOSUB fn_putstr
        #lit_addr = VARPTR lit_tables(0) : GOSUB put_lit
        GOSUB net_open_settled
        bt_n = 0
        IF fn_ok THEN
            #net_readlen = 1 + 9 * TB_STRIDE
            IF #net_avail < #net_readlen THEN #net_readlen = #net_avail
            GOSUB net_read
            IF fn_ok AND #net_gotlen > 0 THEN
                bt_n = PEEK(FN_RX) AND 255
                IF bt_n > 9 THEN bt_n = 9
                IF 1 + bt_n * TB_STRIDE > #net_gotlen THEN bt_n = (#net_gotlen - 1) / TB_STRIDE
                CALL COPYB(FN_RX + 1, TB_LIST, bt_n * TB_STRIDE)
            END IF
        END IF
        GOSUB net_close
        t_pos = 220 : GOSUB clear_row
        IF bt_n = 0 THEN
            PRINT AT 220 COLOR C_RED, "NO TABLES: BUTTON"
            DO : WAIT : GOSUB take_input : LOOP UNTIL ev_btn
        ELSE
            FOR bt_i = 0 TO bt_n - 1
                #t_src = TB_LIST + bt_i * TB_STRIDE + 8 : t_pos = (bt_i + 2) * 20 + 1 : t_len = 13 : #t_col = C_WHITE
                GOSUB put_ram
                #t_src = TB_LIST + bt_i * TB_STRIDE + 28 : t_pos = (bt_i + 2) * 20 + 15 : t_len = 5 : #t_col = C_TAN
                GOSUB put_ram
            NEXT bt_i
            PRINT AT 220 COLOR C_WHITE, "DISC, THEN BUTTON"
            bt_sel = 0
            DO
                WAIT
                GOSUB take_input
                FOR bt_i = 0 TO bt_n - 1
                    IF bt_i = bt_sel THEN #BACKTAB((bt_i + 2) * 20) = (62 - 32) * 8 + C_YELLOW ELSE #BACKTAB((bt_i + 2) * 20) = 0
                NEXT bt_i
                IF ne_rep > 0 THEN ne_rep = ne_rep - 1
                IF ne_rep = 0 THEN
                    IF (inp_dir AND 4) AND bt_sel > 0 THEN bt_sel = bt_sel - 1 : ne_rep = 12 : GOSUB snd_select
                    IF (inp_dir AND 1) AND bt_sel < bt_n - 1 THEN bt_sel = bt_sel + 1 : ne_rep = 12 : GOSUB snd_select
                END IF
            LOOP UNTIL ev_btn
            FOR bt_i = 0 TO 7
                POKE SC_TABLE + bt_i, PEEK(TB_LIST + bt_sel * TB_STRIDE + bt_i) AND 255
            NEXT bt_i
            POKE SC_TABLE + 8, 0
            IF PEEK(SC_TABLE) AND 255 THEN EXIT WHILE
        END IF
    WEND
    GOSUB snd_select
END
