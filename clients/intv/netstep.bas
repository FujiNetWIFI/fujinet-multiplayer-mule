' ===========================================================================
' netstep.bas -- net_exchange's request, a step a frame (net.bas).
'
' fn_transact posts a transaction to the cart and waits for its echo, so
' the main loop stood still for each of them, the HTTP GET included (FujiNet
' makes it inside the first STATUS). Here a transaction is posted (fn_post)
' and the main loop goes on; net_tick looks for the echo (fn_check) once a
' frame and posts the next: OPEN, a STATUS until the reply settles (two
' equal non-zero counts, at most 20), the first 512 bytes, the rest, CLOSE.
' NX_ST says which is out (1-5). The globals net_exchange uses hold the
' rest between frames.
'
' net_exchange (the door in develop, leaving, boot) still waits: it first
' lets a request on its way finish (net_finish), whose state it then
' replaces. The stack is as deep as it was: net_step does its own posting.
' fn_post and fn_check are in netpost.bas (segment 3: segment 1 is full).
' ===========================================================================

' net_tick: the main loop's network step: the request going on, or the
' next one begun when it's time. (Not in test mode: net_poll's test_poll.)
net_tick: PROCEDURE
    IF PEEK(NX_ST) AND 255 THEN
        GOSUB net_step
        IF (PEEK(NX_ST) AND 255) = 0 THEN GOSUB net_react
    ELSEIF poll_wait = 0 OR act <> ACT_NONE THEN
        net_have = 0
        IF act = ACT_NONE THEN act = ACT_STATE
        GOSUB compose
        act_sent = act
        act = ACT_NONE
        mb_dev = NET_DEVICEID : mb_cmd = NETCMD_OPEN : mb_nparam = 2
        pm_i = 0 : pm_size = 1 : #pm_val = OPEN_MODE_HTTP_GET_H : GOSUB fn_param
        pm_i = 1 : pm_size = 1 : #pm_val = OPEN_TRANS_NONE : GOSUB fn_param
        GOSUB fn_post
        POKE NX_ST, 1
    END IF
END

' net_react: net_poll's ending (mule.bas), for net_tick: here, as IntyBASIC
' won't let the two share it, and calling it from net_poll would make the
' door's exchange in develop a word deeper on a stack that has none to spare.
net_react: PROCEDURE
    IF net_have = 1 THEN
        net_fail = 0
        GOSUB state_arrived
    ELSEIF net_have = 2 THEN
        ' The server refused us outright (unknown table, table full...).
        IF st8(ST_ERR) = ERR_TABLEFULL OR st8(ST_ERR) = ERR_SEAT THEN
            GOSUB clear_room_appkey
            GOSUB table_select
            GOSUB write_room_appkey
            last_phase = 255
        END IF
    ELSE
        net_fail = net_fail + 1
    END IF
    GOSUB poll_rate
END

' net_finish: a request on its way, to its end. Its state is neither copied
' nor acted on (the exchange that follows replaces it): no COPYB, so the
' door's exchange in develop is no deeper than it was.
net_finish: PROCEDURE
    POKE NX_SK, 1
    WHILE PEEK(NX_ST) AND 255
        WAIT
        GOSUB net_step
    WEND
    POKE NX_SK, 0
END

' net_step: the cart's answer to what is out, if it has come, and the next.
net_step: PROCEDURE
    GOSUB fn_check
    IF fn_ok = 255 THEN RETURN
    IF (PEEK(NX_ST) AND 255) = 5 THEN POKE NX_ST, 0 : RETURN     ' closed: done
    IF fn_ok = 0 THEN GOTO nx_close

    IF (PEEK(NX_ST) AND 255) = 1 THEN
        ' opened: the first STATUS
        #ac_prev = 0 : ac_i = 0
    ELSEIF (PEEK(NX_ST) AND 255) = 2 THEN
        ' a STATUS: settled yet?
        #net_avail = (PEEK(FN_RX) AND 255) + (PEEK(FN_RX + 1) AND 255) * 256
        IF (PEEK(FN_RX + 3) AND 255) <> 1 THEN GOTO nx_close
        ac_i = ac_i + 1
        IF (#net_avail = 0 OR #net_avail <> #ac_prev) AND ac_i < 20 THEN
            #ac_prev = #net_avail
        ELSE
            ' the first piece: at most 512 bytes (the mailbox window)
            #net_n = #net_avail
            #net_readlen = 512
            IF #net_n < 512 THEN #net_readlen = #net_n
            GOTO nx_read
        END IF
    ELSE
        #net_gotlen = (PEEK(FN_RXLEN_LO) AND 255) + (PEEK(FN_RXLEN_HI) AND 255) * 256
        IF (PEEK(NX_ST) AND 255) = 3 THEN
            IF #net_gotlen < SZ_HEADER THEN GOTO nx_close
            IF (PEEK(FN_RX + ST_MAGIC) AND 255) <> 77 THEN GOTO nx_close
            IF (PEEK(FN_RX + ST_MSG_TYPE) AND 255) <> MSG_STATE THEN
                ' Error header only: keep it for the caller (ST_ERR says why).
                IF PEEK(NX_SK) AND 255 THEN GOTO nx_close
                CALL COPYB(FN_RX, STATE, SZ_HEADER)
                net_have = 2
                GOTO nx_close
            END IF
            IF #net_gotlen < 512 AND #net_gotlen < SZ_STATE THEN GOTO nx_close
            IF PEEK(NX_SK) AND 255 THEN GOTO nx_close
            CALL COPYB(FN_RX, STATE, #net_gotlen)
            #net_i = #net_gotlen
            IF #net_i < SZ_STATE THEN
                ' the rest
                #net_readlen = SZ_STATE - #net_i
                GOTO nx_read
            END IF
        ELSE
            IF #net_gotlen < SZ_STATE - #net_i THEN GOTO nx_close
            IF PEEK(NX_SK) AND 255 THEN GOTO nx_close
            CALL COPYB(FN_RX, STATE + #net_i, #net_gotlen)
        END IF
        net_have = 1
        GOTO nx_close
    END IF

    ' (a STATUS)
    mb_dev = NET_DEVICEID : mb_cmd = NETCMD_STATUS : mb_nparam = 2
    pm_i = 0 : pm_size = 1 : #pm_val = 0 : GOSUB fn_param
    pm_i = 1 : pm_size = 1 : #pm_val = 0 : GOSUB fn_param
    #fn_txlen = 0
    GOSUB fn_post
    POKE NX_ST, 2
    RETURN

nx_read:
    mb_dev = NET_DEVICEID : mb_cmd = NETCMD_READ : mb_nparam = 1
    pm_i = 0 : pm_size = 2 : #pm_val = #net_readlen : GOSUB fn_param
    #fn_txlen = 0
    GOSUB fn_post
    POKE NX_ST, (PEEK(NX_ST) AND 255) + 1
    RETURN

nx_close:
    mb_dev = NET_DEVICEID : mb_cmd = NETCMD_CLOSE : mb_nparam = 0
    #fn_txlen = 0
    GOSUB fn_post
    POKE NX_ST, 5
END
