' ===========================================================================
' memory.bas -- RAM map, colours and the global variables every module shares.
'
' IntyBASIC gives 228 8-bit and 55 16-bit variables. Anything tabular lives in
' the cartridge RAM at $8000-$9BFF (8 bits per address) and is reached with
' PEEK/POKE; see fujinet.bas for why that window stops at $9BFF.
' ===========================================================================

    ' ---- cartridge RAM ------------------------------------------------------
    CONST STATE     = $8000     ' the last good MuleState (SZ_STATE bytes)
    CONST PREV      = $8240     ' the state drawn last (for redraw diffs)
    CONST TXT       = $8700     ' the message line's text, 128 bytes (it scrolls)
    CONST TTL       = $8780     ' a title's text, 32 bytes
    ' fx.bas owns $8600-$8643 (FXB), input.bas $8FF0-$8FF4 (TEST_HOOK).
    ' fujinet.bas owns $9100-$917F (SC_NAME, SC_ENDPT, SC_QUERY).
    CONST SC_TABLE  = $9180     ' table id, 8 chars + NUL
    ' bar_tick's state, in cartridge RAM (map.bas; the 8-bit variables
    ' are full)
    CONST SND_TK = $8814        ' 1: the sound playing is a tick
    CONST TK_N = $8815          ' frames to the next tick
    CONST TK_G = $8816          ' 1: the next tick is the low one
    CONST TK_F = $8817          ' FRAME AND 255 when last run
    CONST TK_U = $8818          ' the bar's units left (the caller's)

    ' ---- Intellivision colours ----------------------------------------------
    CONST C_BLACK = 0
    CONST C_BLUE = 1
    CONST C_RED = 2
    CONST C_TAN = 3
    CONST C_DKGREEN = 4
    CONST C_GREEN = 5
    CONST C_YELLOW = 6
    CONST C_WHITE = 7
    CONST C_GREY = 8
    CONST C_CYAN = 9
    CONST C_ORANGE = 10
    CONST C_BROWN = 11
    CONST C_PINK = 12
    CONST C_LTBLUE = 13
    CONST C_YELGREEN = 14
    CONST C_PURPLE = 15

    ' ---- screens ------------------------------------------------------------
    CONST SCR_NONE = 0
    CONST SCR_TEXT = 1          ' plain text screen (boot, table list, lobby)
    CONST SCR_MAP = 2
    CONST SCR_TOWN = 3
    CONST SCR_AUCTION = 4       ' per-good status / declare / trading, land auction
    CONST SCR_SUMMARY = 5
    CONST SCR_FOUNDER = 6

    ' ---- what the frame handler animates (anim) ------------------------------
    CONST AN_NONE = 0
    CONST AN_WALK = 1           ' our own development turn: map or town
    CONST AN_GRANT = 2          ' land grant cursor
    CONST AN_AUCTION = 3        ' auction lines
    CONST AN_FX = 4             ' scripted animation (ship, events, walk-ins)

    ' ---- globals ------------------------------------------------------------
    ' Clock. #jiffy counts frames since boot (the frame handler bumps it).
    ' #rx_jiffy is the frame the last state arrived; phase-elapsed "now" is
    ' #rx_pe + (#jiffy - #rx_jiffy).
    DIM #rx_jiffy, #rx_pe
    DIM #ph_t0                  ' a frame count a phase's animation started at
    DIM phase, round, my_seat, last_phase, last_round
    DIM scr_now                 ' SCR_* currently on the TV
    DIM anim                    ' AN_*, read by the frame handler
    DIM st_ok                   ' 1 once a valid state has arrived
    DIM poll_wait               ' frames until the next /state poll
    DIM net_fail                ' consecutive failed polls

    ' Scratch shared by the drawing helpers (never used by the frame handler).
    DIM #addr, #val, #val2, #w
    DIM i, j, k, n, c, x, y

    ' The shared helpers' own scratch (screen.bas, net.bas, map.bas), so a
    ' helper never disturbs the loop variables of whoever called it.
    DIM h_i, h_c, h_k, #h_w, #h_a

    ' Frame-handler scratch (never used by the main loop).
    DIM ix, iy, ii, ic, ia, #iw

    ' Scripted animations (fx.bas): shared by both sides.
    DIM fx_on, fx_flag, fa, fop, fmx, fmy
    DIM #fx_pc(4), #fx_f(4)

    ' ---- helpers (DEF FN is textual, so these must come before any use) ----
    ' Background bits of a FG/BG-mode word for colour c (0..15).
    DEF FN fgbg_bg(c) = (((c) AND 3) * $200 + ((c) AND 4) * $800 + ((c) AND 8) * $200)

    ' State accessors (STATE is 8-bit RAM; mask what PEEK returns).
    DEF FN st8(o) = (PEEK(STATE + (o)) AND 255)
    DEF FN st16(o) = ((PEEK(STATE + (o)) AND 255) + (PEEK(STATE + (o) + 1) AND 255) * 256)
    DEF FN pl8(s, o) = (PEEK(STATE + ST_PLAYERS + (s) * SZ_PLAYER + (o)) AND 255)
    DEF FN pl16(s, o) = ((PEEK(STATE + ST_PLAYERS + (s) * SZ_PLAYER + (o)) AND 255) + (PEEK(STATE + ST_PLAYERS + (s) * SZ_PLAYER + (o) + 1) AND 255) * 256)

    ' Phase-elapsed jiffies, estimated between polls.
    DEF FN pe_now = (#rx_pe + (FRAME - #rx_jiffy))

    ' A GRAM card as a SPRITE f word or colour-stack BACKTAB word, colour c.
    DEF FN gram_word(slot, c) = ($0800 + (slot) * 8 + ((c) AND 7) + ((c) AND 8) * $200)
