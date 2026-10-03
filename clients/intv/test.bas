' ===========================================================================
' test.bas -- an offline test mode for tools/intvsim.py.
'
' When the harness pokes $5A at TEST_HOOK + 2 before boot, the game skips
' FujiNet and fakes a state: round 1, our development turn on a generated
' map (TEST_HOOK + 3 picks the phase: 3 develop, 4 production with the
' colony event in TEST_HOOK + 4, 5/6 declare/trading with the auction stage
' in TEST_HOOK + 4, 7 summary). Actions are answered locally
' so walking, the town, events and animations can be checked in seconds.
' ===========================================================================

    DIM test_mode

test_start:
    test_mode = 1
    FOR #addr = 0 TO SZ_STATE - 1 : POKE STATE + #addr, 0 : NEXT #addr
    POKE STATE + ST_MAGIC, 77
    POKE STATE + ST_VERSION, MULE_PROTO_VERSION
    POKE STATE + ST_MSG_TYPE, MSG_STATE
    k = PEEK(TEST_HOOK + 3) AND 255
    IF k = 0 THEN k = PHASE_DEVELOP
    POKE STATE + ST_PHASE, k
    POKE STATE + ST_ROUND, 1
    POKE STATE + ST_MY_SEAT, 0
    FOR i = 0 TO 3
        #addr = STATE + ST_PLAYERS + i * SZ_PLAYER
        POKE #addr, 80 + i : POKE #addr + 1, 76 : POKE #addr + 2, 48 + i   ' "PL0".. names
        ' the summary takes TEST_HOOK + 4 as a species offset: 1 shows the other four
        IF k = PHASE_SUMMARY THEN j = PEEK(TEST_HOOK + 4) ELSE j = 0
        POKE #addr + S_PLAYER_SPECIES, (1 + i * 2 + j) AND 7
        POKE #addr + S_PLAYER_COLOR, i
        POKE #addr + S_PLAYER_MONEY, 232 : POKE #addr + S_PLAYER_MONEY + 1, 3   ' 1000
        POKE #addr + S_PLAYER_RANK, i + 1
        POKE #addr + S_PLAYER_GOODS, 1 + i * 2     ' food 1/3/5/7: short, even, over
    NEXT i
    POKE STATE + ST_PLAYERS + 3 * SZ_PLAYER + S_PLAYER_FLAGS, PF_AI
    ' The map: the river down column 4, the store in the middle, a mountain
    ' plot either side in each row, plots 10, 12, 14, 20, 30, 34 owned.
    FOR mp = 0 TO 44
        c = TERRAIN_PLAIN
        IF mp % 9 = 4 THEN c = TERRAIN_RIVER
        IF mp = STORE_PLOT THEN c = TERRAIN_STORE
        IF mp % 9 = (mp / 9) % 4 THEN c = TERRAIN_MOUNTAIN + ((mp / 9) % 3 + 1) * 4
        IF mp % 9 = 5 + (mp / 9) % 4 THEN c = TERRAIN_MOUNTAIN + ((mp / 9 + 1) % 3 + 1) * 4
        POKE STATE + ST_MAP_TERRAIN + mp, c
        POKE STATE + ST_MAP_OWNER + mp, 255
        POKE STATE + ST_MAP_MULE + mp, 255
        POKE STATE + ST_MAP_CRYSTITE + mp, 255
    NEXT mp
    POKE STATE + ST_MAP_OWNER + 10, 0 : POKE STATE + ST_MAP_MULE + 10, GOOD_FOOD
    POKE STATE + ST_MAP_PROD + 10, 5
    POKE STATE + ST_MAP_OWNER + 12, 1 : POKE STATE + ST_MAP_MULE + 12, GOOD_ENERGY
    POKE STATE + ST_MAP_PROD + 12, 3
    POKE STATE + ST_MAP_OWNER + 30, 3 : POKE STATE + ST_MAP_MULE + 30, GOOD_ENERGY
    POKE STATE + ST_MAP_OWNER + 20, 0 : POKE STATE + ST_MAP_MULE + 20, GOOD_SMITHORE
    POKE STATE + ST_MAP_PROD + 20, 8
    POKE STATE + ST_MAP_OWNER + 34, 2 : POKE STATE + ST_MAP_MULE + 34, GOOD_CRYSTITE
    POKE STATE + ST_MAP_PROD + 34, 2
    POKE STATE + ST_STORE_MULES, 14 : POKE STATE + ST_STORE_MULE_PRICE, 100
    FOR i = 0 TO 3 : POKE STATE + ST_STORE_PRICE + i * 2, 20 + i * 20 : NEXT i
    ' Our turn (or seat TEST_HOOK + 4's), started, 2828 jiffies long, a
    ' wampus on plot 0 early on.
    POKE STATE + ST_TURN_SEAT, PEEK(TEST_HOOK + 4) AND 3
    POKE STATE + ST_TURN_FLAGS, TF_STARTED
    POKE STATE + ST_TURN_LENGTH, 2828 AND 255 : POKE STATE + ST_TURN_LENGTH + 1, 2828 / 256
    POKE STATE + ST_TURN_LEFT, 2828 AND 255 : POKE STATE + ST_TURN_LEFT + 1, 2828 / 256
    POKE STATE + ST_TURN_P_TU, 28
    POKE STATE + ST_TURN_CARRYING, 255
    POKE STATE + ST_TURN_EVENT, PEEK(TEST_HOOK + 5) AND 255
    POKE STATE + ST_TURN_N_WAMPUS, 1
    POKE STATE + ST_TURN_WAMPUS + S_WAMPUS_PLOT, 0
    POKE STATE + ST_TURN_WAMPUS + S_WAMPUS_TO, 255
    POKE STATE + ST_TURN_WAMPUS + S_WAMPUS_TO + 1, 10
    ' Production / colony event.
    POKE STATE + ST_COLONY_EVENT, PEEK(TEST_HOOK + 4) AND 255
    POKE STATE + ST_COLONY_PLOT, 10
    ' Auctions.
    POKE STATE + ST_AUCTION_KIND, 2
    POKE STATE + ST_AUCTION_STEP, 1
    IF k = PHASE_LANDAUCTION THEN
        POKE STATE + ST_AUCTION_KIND, 1 : POKE STATE + ST_AUCTION_STEP, 4
        POKE STATE + ST_AUCTION_START, 240 : POKE STATE + ST_AUCTION_PLOT, 13
    END IF
    POKE STATE + ST_AUCTION_STORE_BUY, 30
    POKE STATE + ST_AUCTION_STORE_SELL, 65
    FOR i = 0 TO 3
        POKE STATE + ST_AUCTION_ROLE + i, ROLE_BUYER
        POKE STATE + ST_AUCTION_POS + i * 2, 20 + i * 5
        POKE STATE + ST_AUCTION_TARGET + i * 2, 40 + i * 5
    NEXT i
    POKE STATE + ST_AUCTION_ROLE + 1, ROLE_SELLER
    POKE STATE + ST_AUCTION_TICKS, 70
    POKE STATE + ST_AUCTION_STAGE, PEEK(TEST_HOOK + 4) AND 255
    POKE STATE + ST_AUCTION_HI, 66 : POKE STATE + ST_AUCTION_LO, 20
    FOR i = 0 TO 3
        POKE STATE + ST_PLAYERS + i * SZ_PLAYER + S_PLAYER_PROD, 3 + i
        POKE STATE + ST_PLAYERS + i * SZ_PLAYER + S_PLAYER_SPOIL, i
    NEXT i
    GOTO game_start

' test_poll: answer an action as the server would, roughly.
test_poll: PROCEDURE
    net_have = 1
    act_sent = act
    POKE STATE + ST_ERR, 0
    IF act = ACT_BUYMULE THEN POKE STATE + ST_TURN_FLAGS, TF_STARTED + TF_HASMULE
    IF act = ACT_RETURN THEN POKE STATE + ST_TURN_FLAGS, TF_STARTED
    IF act = ACT_OUTFIT THEN POKE STATE + ST_TURN_CARRYING, #act_arg
    IF act = ACT_INSTALL THEN
        POKE STATE + ST_TURN_FLAGS, TF_STARTED
        POKE STATE + ST_MAP_OWNER + #act_arg, 0
        POKE STATE + ST_MAP_MULE + #act_arg, PEEK(STATE + ST_TURN_CARRYING)
    END IF
    IF act = ACT_ASSAY THEN POKE STATE + ST_RESULT, 3
    IF act = ACT_PUB THEN POKE STATE + ST_TURN_FLAGS, TF_STARTED + TF_ENDED : POKE STATE + ST_RESULT, 75
    ' The clock: the turn runs down, the phase ages.
    #w = FRAME - #rx_jiffy
    #val = st16(ST_TURN_LEFT)
    IF st8(ST_PHASE) = PHASE_DEVELOP AND #val > #w THEN #val = #val - #w ELSE #val = 0
    POKE STATE + ST_TURN_LEFT, #val AND 255 : POKE STATE + ST_TURN_LEFT + 1, #val / 256
    #val = st16(ST_ELAPSED) + #w
    POKE STATE + ST_ELAPSED, #val AND 255 : POKE STATE + ST_ELAPSED + 1, #val / 256
    #val = st16(ST_TURN_ELAPSED) + #w
    POKE STATE + ST_TURN_ELAPSED, #val AND 255 : POKE STATE + ST_TURN_ELAPSED + 1, #val / 256
    act = ACT_NONE
    GOSUB state_arrived
END

