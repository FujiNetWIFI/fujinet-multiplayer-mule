' ===========================================================================
' map.bas -- the planet map: 9 x 5 plots, each 2 x 2 cards.
'
' Layout (docs/ANIMATION.md): row 0 is the title, rows 1..10 the map, row 11
' the message line. Plot column c sits at card column 1 + 2c, which is where
' the Atari's mode 6 characters put it, so nothing is rescaled across.
'
' A plot's four cards come from the map card set (gen/gfx.bas):
'   top-left     border (if owned) + 0..3 mountains
'   top-right    border + icon top (house, or the M.U.L.E.'s good)
'   bottom-left  border + production dots (production phase only)
'   bottom-right border + icon bottom
' Owned plots are drawn in the owner's colour, mountains on free land in
' black. The river is three MOBs behind the cards, so borders and the store
' cover it the way the Atari's playfield covers player 3.
' ===========================================================================

    CONST MOUNT_COL = C_BLACK
    CONST RIVER_COL = C_BLUE       ' blue, as player 0's (C_LTBLUE is lost on the grey)

    DIM map_dots                ' 1 = draw production dots
    DIM map_blink               ' plot for sale (blinks), 255 = none
    DIM map_blink_on            ' 1 = draw it outlined in white now
    DIM mp, mo, mt, mm, mg, mc
    DIM bar_col, bar_rows, bar_px, bar_last     ' the time bar (bar_draw)
    DIM bar_colr                ' its colour (0..15; 0..7 in the town)

' map_enter: switch to the map screen and draw everything.
map_enter: PROCEDURE
    anim = AN_NONE
    GOSUB hide_mobs
    scr_bg = C_GREY : GOSUB screen_stack
    g_slot = 0 : g_n = MC_COUNT : #g_src = VARPTR gfx_map(0) : GOSUB gram_load
    g_slot = G_BAR : g_n = 8 : #g_src = VARPTR gfx_bar(0) : GOSUB gram_load
    g_slot = G_MULE : g_n = 2 : #g_src = VARPTR gfx_mule(0) : GOSUB gram_load
    g_slot = G_WAMPUS : g_n = 2 : #g_src = VARPTR gfx_cursor(0) : GOSUB gram_load
    scr_now = SCR_MAP
    ' The map's time bar (the town's is at column 17 and would cover plot
    ' column 8); bar_last = 255 redraws it on the cleared screen.
    bar_col = 19 : bar_rows = 10 : bar_last = 255
    map_blink = 255 : map_blink_on = 0
    GOSUB map_draw_all
    GOSUB river_show
END

' The river: column 4, drawn behind the cards (docs/ANIMATION.md).
river_show: PROCEDURE
    #w = gram_word(MC_RIVER, RIVER_COL) + BEHIND
    SPRITE MOB_RIVER, 84 + VISIBLE, 16 + DOUBLEY + ZOOMY4, #w
    SPRITE MOB_RIVER + 1, 84 + VISIBLE, 48 + DOUBLEY + ZOOMY4, #w
    SPRITE MOB_RIVER + 2, 84 + VISIBLE, 80 + ZOOMY4, #w
END

map_draw_all: PROCEDURE
    FOR mp = 0 TO 44
        GOSUB map_draw_plot
    NEXT mp
END

' map_draw_plot: draw plot mp from STATE.
map_draw_plot: PROCEDURE
    #addr = ((mp / 9) * 2 + 1) * 20 + (mp % 9) * 2 + 1
    mt = st8(ST_MAP_TERRAIN + mp)
    IF (mt AND 3) = TERRAIN_STORE THEN
        #BACKTAB(#addr) = gram_word(MC_STORE, C_BLACK)
        #BACKTAB(#addr + 1) = gram_word(MC_STORE + 1, C_BLACK)
        #BACKTAB(#addr + 20) = gram_word(MC_STORE + 2, C_BLACK)
        #BACKTAB(#addr + 21) = gram_word(MC_STORE + 3, C_BLACK)
        RETURN
    END IF
    mm = (mt / 4) AND 3
    mo = st8(ST_MAP_OWNER + mp)
    IF mp = map_blink AND map_blink_on THEN
        ' Up for auction, or a computer working it: outlined in white, with
        ' its icon (the house, or its M.U.L.E.'s good).
        mg = st8(ST_MAP_MULE + mp)
        IF mo < 4 AND mg < 4 THEN mg = mg + 1 ELSE mg = 0
        #BACKTAB(#addr) = gram_word(MC_TLO + mm, C_WHITE)
        #BACKTAB(#addr + 1) = gram_word(MC_TR + mg, C_WHITE)
        #BACKTAB(#addr + 20) = gram_word(MC_BL, C_WHITE)
        #BACKTAB(#addr + 21) = gram_word(MC_BR + mg, C_WHITE)
        RETURN
    END IF
    IF mo > 3 THEN
        IF mm THEN #BACKTAB(#addr) = gram_word(MC_TLU + mm - 1, MOUNT_COL) ELSE #BACKTAB(#addr) = 0
        #BACKTAB(#addr + 1) = 0
        #BACKTAB(#addr + 20) = 0
        #BACKTAB(#addr + 21) = 0
        RETURN
    END IF
    mc = pl_mobcol(pl8(mo, S_PLAYER_COLOR) AND 3)
    mg = st8(ST_MAP_MULE + mp)
    IF mg < 4 THEN mg = mg + 1 ELSE mg = 0      ' icon: 0 house, 1.. goods
    #BACKTAB(#addr) = gram_word(MC_TLO + mm, mc)
    #BACKTAB(#addr + 1) = gram_word(MC_TR + mg, mc)
    k = 0
    IF map_dots AND mg > 0 THEN
        k = st8(ST_MAP_PROD + mp)
        IF k AND 128 THEN k = 0 ELSE k = k AND 15
        IF k > 8 THEN k = 8
    END IF
    #BACKTAB(#addr + 20) = gram_word(MC_BL + k, mc)
    #BACKTAB(#addr + 21) = gram_word(MC_BR + mg, mc)
END

' map_update: redraw the plots whose owner, M.U.L.E., terrain or production
' changed since PREV.
map_update: PROCEDURE
    FOR mp = 0 TO 44
        k = 0
        IF (PEEK(STATE + ST_MAP_TERRAIN + mp) XOR PEEK(PREV + ST_MAP_TERRAIN + mp)) AND 255 THEN k = 1
        IF (PEEK(STATE + ST_MAP_OWNER + mp) XOR PEEK(PREV + ST_MAP_OWNER + mp)) AND 255 THEN k = 1
        IF (PEEK(STATE + ST_MAP_MULE + mp) XOR PEEK(PREV + ST_MAP_MULE + mp)) AND 255 THEN k = 1
        IF (PEEK(STATE + ST_MAP_PROD + mp) XOR PEEK(PREV + ST_MAP_PROD + mp)) AND 255 THEN k = 1
        IF k THEN GOSUB map_draw_plot
    NEXT mp
END

' ---------------------------------------------------------------------------
' Title (row 0) and message (row 11) lines. Titles are centred; a message
' that fits is centred, a longer one scrolls from the right a character every
' 8 frames (the Atari's fine scroll moves a character in 8).
' ---------------------------------------------------------------------------
    CONST ROW_TITLE = 0
    CONST ROW_MSG = 220
    DIM msg_id, msg_len, msg_pos, msg_row
    DIM #msg_col
    DIM msg_scroll

' title_show: t_titles entry #val_t (see text.bas) with #val for any '%'.
    DIM title_id
    DIM #te_buf                 ' where text_expand_to writes
title_show: PROCEDURE
    t_pos = ROW_TITLE : GOSUB clear_row
    #lit_addr = VARPTR t_titles(0) : lit_n = title_id : GOSUB lit_nth
    #t_src = #lit_addr : #t_col = C_BLACK
    #te_buf = TTL : GOSUB text_expand_to
    t_pos = ROW_TITLE + (20 - t_len) / 2
    #t_src = TTL : GOSUB put_ram
END

' text_expand: copy length-prefixed DATA string #t_src into TXT as ASCII,
' replacing '%' with #val (first) and #val2 (second). t_len = new length.
' text_expand_to does the same into te_buf (TTL, for titles, so a scrolling
' message in TXT keeps going).
    DIM te_n, te_i, te_c
text_expand: PROCEDURE
    #te_buf = TXT
    GOSUB text_expand_to
END

text_expand_to: PROCEDURE
    te_n = PEEK(#t_src)
    t_len = 0
    #h_w = #val
    FOR te_i = 1 TO te_n
        te_c = PEEK(#t_src + te_i) + 32
        IF te_c = 37 THEN           ' '%'
            #h_a = 10000 : h_k = 0
            WHILE #h_a > 0
                h_c = (#h_w / #h_a) % 10
                IF h_c > 0 OR h_k OR #h_a = 1 THEN
                    POKE #te_buf + t_len, h_c + 48 : t_len = t_len + 1 : h_k = 1
                END IF
                #h_a = #h_a / 10
            WEND
            #h_w = #val2
        ELSE
            POKE #te_buf + t_len, te_c : t_len = t_len + 1
        END IF
        IF t_len >= 124 THEN EXIT FOR
    NEXT te_i
    POKE #te_buf + t_len, 0
END

' msg_show: message msg_id (t_msgs) in #msg_col on msg_row, with #val/#val2.
' A message longer than 20 characters scrolls (msg_tick_run drives it).
msg_show: PROCEDURE
    #lit_addr = VARPTR t_msgs(0) : lit_n = msg_id : GOSUB lit_nth
    #t_src = #lit_addr
    GOSUB text_expand
    GOSUB msg_start
END

' msg_start: show the text already in TXT (t_len long).
msg_start: PROCEDURE
    msg_len = t_len
    t_pos = msg_row : GOSUB clear_row
    IF msg_len <= 20 THEN
        msg_scroll = 0
        t_pos = msg_row + (20 - msg_len) / 2 : #t_col = #msg_col : t_len = 20
        #t_src = TXT : GOSUB put_ram
    ELSE
        ' Scroll: the text enters from the right, then repeats after a gap.
        msg_scroll = 1 : msg_pos = 0
    END IF
END

' msg_tick_run: called every main-loop frame to advance a scrolling message.
    DIM #msg_last
msg_tick_run: PROCEDURE
    IF msg_scroll = 0 THEN RETURN
    IF FRAME - #msg_last < 8 THEN RETURN
    #msg_last = FRAME
    ' Window: columns 0..19 show characters msg_pos-20 .. msg_pos-1.
    FOR h_k = 0 TO 19
        #h_a = msg_pos + h_k
        IF #h_a >= 20 AND #h_a < 20 + msg_len THEN
            h_c = PEEK(TXT + #h_a - 20) AND 255
            #BACKTAB(msg_row + h_k) = (h_c - 32) * 8 + #msg_col
        ELSE
            #BACKTAB(msg_row + h_k) = 0
        END IF
    NEXT h_k
    msg_pos = msg_pos + 1
    IF msg_pos >= msg_len + 24 THEN msg_pos = 0
END

msg_clear: PROCEDURE
    msg_scroll = 0
    t_pos = msg_row : GOSUB clear_row
END

' ---------------------------------------------------------------------------
' Time bar: a column of bar cards, full height 80 px (the map) or 72 px (the
' town), one pixel for every 2 PTU of turn time left (the Atari's bar loses a
' scanline per PTU; two scanlines are one pixel here).
' ---------------------------------------------------------------------------
bar_draw: PROCEDURE                ' bar_px = height in pixels
    IF bar_px = bar_last THEN RETURN
    bar_last = bar_px
    h_k = bar_px
    FOR h_i = 1 TO bar_rows
        #h_a = (bar_rows + 1 - h_i) * 20 + bar_col
        IF h_k >= 8 THEN
            #h_w = gram_word(G_BAR + 7, bar_colr)
            h_k = h_k - 8
        ELSEIF h_k > 0 THEN
            #h_w = gram_word(G_BAR + h_k - 1, bar_colr)
            h_k = 0
        ELSE
            IF scr_mode = 1 THEN #h_w = fgbg_bg(scr_bg) ELSE #h_w = 0
        END IF
        IF scr_mode = 1 AND #h_w AND $0800 THEN #h_w = (#h_w AND $0FF8) + (bar_colr AND 7) + fgbg_bg(scr_bg)
        #BACKTAB(#h_a) = #h_w
    NEXT h_i
END

' ---------------------------------------------------------------------------
' The time bar's tick, the original's (its VBI, $5E98): with tk_u units of
' the bar left (PTU on a turn, ticks in an auction) and tk_u under 30, a
' tick every tk_u frames, high and low by turns (~2900 and ~2000 Hz), so it
' hurries as the bar runs out. Soft, and it cuts only another tick. Run
' every frame (develop_tick, auction_tick); it counts the frames gone by.
' The sound's half: sound.bas (SND_TK and the TK_ state).
' ---------------------------------------------------------------------------
bar_tick_dv: PROCEDURE
    POKE TK_U, 0
    IF (dv_flags AND TF_STARTED) AND (dv_flags AND TF_ENDED) = 0 THEN
        IF FRAME - #rx_jiffy < #dv_left THEN POKE TK_U, (#dv_left - (FRAME - #rx_jiffy)) / dv_ptu
    END IF
    GOSUB bar_tick
END
bar_tick_au: PROCEDURE
    POKE TK_U, 0
    IF au_stage = STAGE_ACTIVE OR au_stage = STAGE_DECLARE THEN POKE TK_U, st8(ST_AUCTION_TICKS)
    GOSUB bar_tick
END
' (#val and k are the effects' scratch: free here)
bar_tick: PROCEDURE
    k = (FRAME - PEEK(TK_F)) AND 255            ' frames gone by
    IF k = 0 THEN RETURN
    POKE TK_F, FRAME AND 255
    #val = PEEK(TK_U)
    IF #val = 0 OR #val >= 30 THEN POKE TK_N, 1 : POKE TK_G, 0 : RETURN
    IF PEEK(TK_N) > k THEN POKE TK_N, PEEK(TK_N) - k : RETURN
    POKE TK_N, #val
    IF snd_t > 0 AND PEEK(SND_TK) = 0 THEN RETURN
    snd_t = 0 : snd_t2 = 0
    IF PEEK(TK_G) THEN SOUND 0, 56, 6 ELSE SOUND 0, 38, 6
    snd_t = 2 : POKE SND_TK, 1
    POKE TK_G, PEEK(TK_G) XOR 1
END
