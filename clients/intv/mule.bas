' ===========================================================================
' mule.bas -- M.U.L.E. (Tournament) for the Mattel Intellivision, played over
' FujiNet against the fujinet-multiplayer-mule server.
'
' Build with IntyBASIC 1.5 (see Makefile). The server runs the game; this
' client polls /state (the 550-byte binary view), draws it the way the Atari
' 800 original did, and sends the player's actions. Everything the client
' animates on its own -- walking, ships, pests, the status summary -- keeps
' the Atari's frame timing (docs/ANIMATION.md).
'
' GPLv3.
' ===========================================================================

    OPTION MAP 2
    OPTION EXPLICIT ON
    ASM CFGVAR "name" = "M.U.L.E. Tournament (FujiNet)"
    ASM CFGVAR "short_name" = "M.U.L.E."
    ASM CFGVAR "year" = 2026

    GOTO boot_entry

    INCLUDE "constants.bas"
    INCLUDE "gen/proto.bas"
    INCLUDE "gen/gfxconst.bas"
    INCLUDE "ids.bas"
    INCLUDE "memory.bas"
    INCLUDE "fujinet.bas"
    INCLUDE "input.bas"
    INCLUDE "net.bas"
    INCLUDE "screen.bas"
    INCLUDE "fx.bas"
    INCLUDE "isr.bas"

    SEGMENT 1
    INCLUDE "map.bas"
    INCLUDE "leave.bas"
    INCLUDE "grant.bas"
    INCLUDE "textlong.bas"

    SEGMENT 2
    INCLUDE "develop.bas"
    INCLUDE "auction.bas"
    INCLUDE "events.bas"
    INCLUDE "status.bas"
    INCLUDE "boot.bas"

    ' $2100-$2FFF: a static segment in MAP 2 (safe on flash carts).
    SEGMENT 3
    INCLUDE "text.bas"
    INCLUDE "sound.bas"

    ' $4810-$4FFF: clear of the ECS (RAM ends at $47FF), the FujiNet RAM and
    ' mailbox, and the GRAM mirrors.
    SEGMENT 5
    INCLUDE "terrain.bas"
    INCLUDE "auneed.bas"
    INCLUDE "test.bas"

    SEGMENT 1
    INCLUDE "gen/gfx.bas"
    ' Segment 2 is full up to $F7FF.
    INCLUDE "gen/config.bas"

    SEGMENT 0

' ---------------------------------------------------------------------------
boot_entry:
    GOSUB fx_clear
    ON FRAME GOSUB frame_isr
    ' tools/intvsim.py can start an offline test (see test.bas).
    IF (PEEK(TEST_HOOK + 2) AND 255) = $5A THEN GOTO test_start
    POKE TEST_HOOK, 0
    GOTO boot_start

' ---------------------------------------------------------------------------
' The game loop: poll (or send the queued action), then per-frame work for
' the current phase.
' ---------------------------------------------------------------------------
game_start:
    POKE LV_GO, 0
    last_phase = 255 : last_round = 255
    scr_now = SCR_NONE
    st_ok = 0
    poll_wait = 0
    act = ACT_NONE
    scr_bg = C_BLACK : GOSUB screen_stack
    PRINT AT 100 COLOR C_WHITE, "JOINING..."

main_loop:
    WAIT
    GOSUB take_input
    IF ev_key = 10 AND st_ok <> 0 AND phase > 0 AND test_mode = 0 THEN GOSUB leave_ask
    IF PEEK(LV_GO) AND 255 THEN POKE LV_GO, 0 : IF test_mode = 0 THEN GOSUB leave_table : GOTO main_loop
    IF poll_wait > 0 THEN poll_wait = poll_wait - 1
    IF poll_wait = 0 OR act <> ACT_NONE THEN GOSUB net_poll
    IF st_ok THEN
        ON phase GOSUB lobby_tick, grant_tick, auction_tick, develop_tick, production_tick, auction_tick, auction_tick, summary_tick, gameover_tick
    END IF
    GOTO main_loop

' net_poll: one exchange with the server, then react to what came back.
net_poll: PROCEDURE
    IF test_mode THEN GOSUB test_poll : GOSUB poll_rate : RETURN
    GOSUB net_exchange
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

' How often to ask: often while things move, rarely while we walk (the
' server has nothing to tell us then) or wait.
poll_rate: PROCEDURE
    IF net_fail THEN poll_wait = 30 : RETURN
    poll_wait = 30
    IF phase = PHASE_GRANT THEN poll_wait = 12
    IF phase = PHASE_LANDAUCTION OR phase = PHASE_DECLARE THEN poll_wait = 12
    IF phase = PHASE_TRADING THEN poll_wait = 8
    IF phase = PHASE_PRODUCTION THEN poll_wait = 60
    IF phase = PHASE_DEVELOP AND dv_state = DV_WALK THEN poll_wait = 60
    IF phase = PHASE_GAMEOVER THEN poll_wait = 60
END

' state_arrived: a full state is in STATE.
state_arrived: PROCEDURE
    k = st8(ST_PHASE)
    IF k > PHASE_GAMEOVER THEN RETURN
    phase = k
    round = st8(ST_ROUND)
    my_seat = st8(ST_MY_SEAT)
    #rx_pe = st16(ST_ELAPSED)
    #rx_jiffy = FRAME
    st_ok = 1
    IF phase <> last_phase OR (phase = PHASE_GRANT AND round <> last_round) THEN
        msg_scroll = 0
        GOSUB fx_stop_all
        anim = AN_NONE
        ON phase GOSUB lobby_enter, grant_enter, landauc_enter, develop_enter, production_enter, goods_enter, goods_enter, summary_enter, gameover_enter
    END IF
    ON phase GOSUB lobby_state, grant_state, auction_state, develop_state, no_state, auction_state, auction_state, summary_state, no_state
    CALL COPYB(STATE, PREV, SZ_STATE)
    last_phase = phase
    last_round = round
END

no_state: PROCEDURE
END

landauc_enter: PROCEDURE
    au_stage = 255 : au_push = 0 : au_traded = 0
    map_dots = 0
END

goods_enter: PROCEDURE
    ' Declare and trading share the screen; a new good starts it afresh.
    IF last_phase <> PHASE_DECLARE AND last_phase <> PHASE_TRADING THEN scr_now = SCR_NONE
    IF phase = PHASE_DECLARE THEN scr_now = SCR_NONE
    au_stage = 255 : au_push = 0 : au_traded = 0
END

develop_enter: PROCEDURE
    dv_state = 255 : dv_msg_seat = 255 : w_mule = 0
    map_dots = 0
    POKE DV_AI, 0
    scr_now = SCR_NONE
END
