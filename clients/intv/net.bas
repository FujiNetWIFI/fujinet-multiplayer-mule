' ===========================================================================
' net.bas -- talking to the M.U.L.E. server.
'
' Every call is an HTTP GET of
'     N:<endpoint><route>[<number>]?table=<t>&player=<name>&bin=1
' and every call, action or poll, returns the complete 550-byte binary state.
' That is bigger than the 512-byte mailbox receive window, so replies are
' read in two pieces (512 + the rest) and copied into STATE.
'
' Only one request is ever in flight. Actions are queued in act/#act_arg and
' take the place of the next /state poll, so they go out as soon as the main
' loop gets to them.
' ===========================================================================

    ' Routes. The order is the ACT_* numbering below; entries that end in '/'
    ' take #act_arg (or the text chosen by act_word) after them.
    CONST ACT_NONE = 0
    CONST ACT_STATE = 1
    CONST ACT_READY = 2
    CONST ACT_UNREADY = 3
    CONST ACT_SPECIES = 4
    CONST ACT_CLAIM = 5
    CONST ACT_PUSH = 6          ' #act_arg: 0 hold, 1 up, 2 down
    CONST ACT_DECLARE = 7       ' #act_arg: 0 sit out, 1 buyer, 2 seller
    CONST ACT_COLLUDE = 8
    CONST ACT_BEGIN = 9
    CONST ACT_BUYMULE = 10
    CONST ACT_OUTFIT = 11       ' #act_arg: GOOD_*
    CONST ACT_INSTALL = 12      ' #act_arg: plot
    CONST ACT_RETURN = 13
    CONST ACT_ASSAY = 14        ' #act_arg: plot
    CONST ACT_SELL = 15         ' #act_arg: plot
    CONST ACT_UNSELL = 16       ' #act_arg: plot
    CONST ACT_WAMPUS = 17
    CONST ACT_PUB = 18
    CONST ACT_END = 19
    CONST ACT_LEAVE = 20
    CONST ACT_VIEW = 21
    CONST ACT_HOLD = 22         ' #act_arg: price our released line stopped at

    ' Default server (the FujiNet lobby's appkey overrides it): gen/config.bas
    ' holds lit_endpoint as a length-prefixed ASCII DATA string.

    DIM act, #act_arg           ' queued action (ACT_NONE = just poll)
    DIM act_sent                ' the action the state in STATE answers
    DIM #net_i, #net_n
    DIM net_have                ' 1 = STATE holds a fresh reply
    DIM au_traded               ' auction units traded so far, as shown (auction.bas); sent with a push

' ---------------------------------------------------------------------------
' put_lit: append DATA-string #lit_addr (length word, then chars - 32) to FN_TX.
' ---------------------------------------------------------------------------
    DIM #lit_addr
put_lit: PROCEDURE
    h_k = PEEK(#lit_addr)
    FOR h_i = 1 TO h_k
        POKE (FN_TX + #fn_txlen), PEEK(#lit_addr + h_i) + 32
        #fn_txlen = #fn_txlen + 1
    NEXT h_i
END

' lit_nth: step #lit_addr forward over lit_n length-prefixed strings.
    DIM lit_n
lit_nth: PROCEDURE
    WHILE lit_n > 0
        #lit_addr = #lit_addr + PEEK(#lit_addr) + 1
        lit_n = lit_n - 1
    WEND
END

' put_num: append #val (0..65535) in decimal.
put_num: PROCEDURE
    #h_w = 10000
    h_k = 0
    WHILE #h_w > 0
        h_c = (#val / #h_w) % 10
        IF h_c > 0 OR h_k OR #h_w = 1 THEN
            POKE (FN_TX + #fn_txlen), h_c + 48
            #fn_txlen = #fn_txlen + 1
            h_k = 1
        END IF
        #h_w = #h_w / 10
    WEND
END

' ---------------------------------------------------------------------------
' compose: build the URL for act (ACT_STATE when nothing is queued) in FN_TX.
' ---------------------------------------------------------------------------
compose: PROCEDURE
    #fn_txlen = 0
    POKE FN_TX, 78 : POKE FN_TX + 1, 58 : #fn_txlen = 2          ' "N:"
    #fn_src = SC_ENDPT : ls_max = 64 : GOSUB fn_strlen : GOSUB fn_putstr

    #lit_addr = VARPTR lit_routes(0) : lit_n = act - 1 : GOSUB lit_nth
    GOSUB put_lit
    IF PEEK(#lit_addr + PEEK(#lit_addr)) = 15 THEN     ' ends in '/': argument
        IF act = ACT_PUSH THEN
            #lit_addr = VARPTR lit_push(0) : lit_n = #act_arg : GOSUB lit_nth : GOSUB put_lit
            ' /seen: the units traded we have shown (server gives back later ones).
            POKE (FN_TX + #fn_txlen), 47 : #fn_txlen = #fn_txlen + 1
            #val = au_traded : GOSUB put_num
        ELSEIF act = ACT_HOLD THEN
            ' /seen/at: the units traded we have shown, where we let go.
            #val = au_traded : GOSUB put_num
            POKE (FN_TX + #fn_txlen), 47 : #fn_txlen = #fn_txlen + 1
            #val = #act_arg : GOSUB put_num
        ELSEIF act = ACT_DECLARE THEN
            #lit_addr = VARPTR lit_declare(0) : lit_n = #act_arg : GOSUB lit_nth : GOSUB put_lit
        ELSE
            #val = #act_arg : GOSUB put_num
        END IF
    END IF

    #lit_addr = VARPTR lit_qtable(0) : GOSUB put_lit
    #fn_src = SC_TABLE : ls_max = 8 : GOSUB fn_strlen : GOSUB fn_putstr
    #lit_addr = VARPTR lit_aplayer(0) : GOSUB put_lit
    #fn_src = SC_NAME : ls_max = 8 : GOSUB fn_strlen : GOSUB fn_putstr
    #lit_addr = VARPTR lit_abin(0) : GOSUB put_lit
END

' ---------------------------------------------------------------------------
' net_exchange: send act (or a poll) and read the reply into STATE.
' net_have = 1 on success. On a server error header (msgType $FF) STATE's
' header is updated but the body is left alone.
' ---------------------------------------------------------------------------
net_exchange: PROCEDURE
    net_have = 0
    IF act = ACT_NONE THEN act = ACT_STATE
    GOSUB compose
    act_sent = act
    act = ACT_NONE
    GOSUB net_open_settled
    IF fn_ok = 0 THEN GOSUB net_close : RETURN
    #net_n = #net_avail

    ' First piece: at most 512 bytes (the mailbox window).
    #net_readlen = 512
    IF #net_n < 512 THEN #net_readlen = #net_n
    GOSUB net_read
    IF fn_ok = 0 OR #net_gotlen < SZ_HEADER THEN GOSUB net_close : RETURN
    IF (PEEK(FN_RX + ST_MAGIC) AND 255) <> 77 THEN GOSUB net_close : RETURN
    IF (PEEK(FN_RX + ST_MSG_TYPE) AND 255) <> MSG_STATE THEN
        ' Error header only: keep it for the caller (ST_ERR says why).
        CALL COPYB(FN_RX, STATE, SZ_HEADER)
        GOSUB net_close
        net_have = 2
        RETURN
    END IF
    IF #net_gotlen < 512 AND #net_gotlen < SZ_STATE THEN GOSUB net_close : RETURN
    CALL COPYB(FN_RX, STATE, #net_gotlen)
    #net_i = #net_gotlen

    ' Second piece: the rest.
    IF #net_i < SZ_STATE THEN
        #net_readlen = SZ_STATE - #net_i
        GOSUB net_read
        IF fn_ok = 0 OR #net_gotlen < SZ_STATE - #net_i THEN GOSUB net_close : RETURN
        CALL COPYB(FN_RX, STATE + #net_i, #net_gotlen)
    END IF
    GOSUB net_close
    net_have = 1
END

' ---------------------------------------------------------------------------
' COPYB(src, dst, count): copy count bytes. An IntyBASIC PEEK/POKE loop
' takes about three frames for a full state; this takes one.
' ---------------------------------------------------------------------------
    ASM COPYB: PROC
    ASM     PSHR R5
    ASM     MOVR R0, R4
    ASM     MOVR R1, R5
    ASM     TSTR R2
    ASM     BEQ @@done
    ASM @@loop:
    ASM     MVI@ R4, R3
    ASM     ANDI #255, R3
    ASM     MVO@ R3, R5
    ASM     DECR R2
    ASM     BNE @@loop
    ASM @@done:
    ASM     PULR PC
    ASM     ENDP

' ---------------------------------------------------------------------------
' String tables (DATA strings are stored as ASCII - 32).
' ---------------------------------------------------------------------------
lit_routes:
    DATA 6, "/state"            ' ACT_STATE
    DATA 8, "/ready/1"          ' ACT_READY
    DATA 8, "/ready/0"          ' ACT_UNREADY
    DATA 15, "/setup/species/" ' ACT_SPECIES
    DATA 7, "/claim/"           ' ACT_CLAIM
    DATA 14, "/auction/push/"   ' ACT_PUSH
    DATA 9, "/declare/"         ' ACT_DECLARE
    DATA 8, "/collude"          ' ACT_COLLUDE
    DATA 11, "/turn/begin"      ' ACT_BEGIN
    DATA 13, "/turn/buymule"    ' ACT_BUYMULE
    DATA 13, "/turn/outfit/"    ' ACT_OUTFIT
    DATA 14, "/turn/install/"   ' ACT_INSTALL
    DATA 12, "/turn/return"     ' ACT_RETURN
    DATA 12, "/turn/assay/"     ' ACT_ASSAY
    DATA 11, "/turn/sell/"      ' ACT_SELL
    DATA 13, "/turn/unsell/"    ' ACT_UNSELL
    DATA 12, "/turn/wampus"     ' ACT_WAMPUS
    DATA 9, "/turn/pub"         ' ACT_PUB
    DATA 9, "/turn/end"         ' ACT_END
    DATA 6, "/leave"            ' ACT_LEAVE
    DATA 5, "/view"             ' ACT_VIEW
    DATA 19, "/auction/push/hold/" ' ACT_HOLD
lit_push:
    DATA 4, "hold"
    DATA 2, "up"
    DATA 4, "down"
lit_declare:
    DATA 1, "n"
    DATA 1, "b"
    DATA 1, "s"
lit_qtable:
    DATA 7, "?table="
lit_aplayer:
    DATA 8, "&player="
lit_abin:
    DATA 6, "&bin=1"
lit_tables:
    DATA 13, "/tables?bin=1"
