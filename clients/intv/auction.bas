' ===========================================================================
' auction.bas -- the per-good status screen, declaring, and both auctions.
'
' Screen (colour stack, grey), from the Atari's display list: rows 0-1 the
' title, rows 2-9 the arena, row 10 money, row 11 units or a message.
'   Atari arena line y -> Intellivision pixel (y - 19) / 2
'   store sell line y 67 -> 24; store buy line / land start y 137 -> 59;
'   one price step = 2 Atari lines = 1 pixel.
' The four players stand in columns at the Atari's HPOS 87/111/135/159
' (MOB x 47/71/95/119), feet on their price line.
' ===========================================================================

    CONST AU_Y_BASE = 59        ' the store's buy line / the land start price
    CONST AU_Y_SELL = 24        ' the store's sell line
    CONST AU_Y_TOP = 18         ' highest line on screen
    CONST AU_Y_BOT = 72         ' lowest line on screen
    CONST G_AU_SP = 0           ' 16 cards: four players' standing frames
    CONST G_AU_SP2 = 16         ' 16 cards: four players' walking frames
    CONST G_AU_HLINE = 32       ' 8 cards: dotted line at pixel row 0..7
    CONST G_AU_SBAR = 48        ' 8 cards: status bars
    CONST G_AU_ICON = 56        ' store icon
    CONST G_AU_FLAG = 57        ' plot for sale
    CONST G_AU_HL2 = 58         ' both price lines in one card row
    CONST AU_LBL = $8806        ' 4 x 2 bytes: what row 9 shows per player
    CONST AU_LN2 = $8805        ' pixel row of the buyers' line (255 none);
                                ' au_ln1 has the sellers' (cart RAM: no
                                ' 8-bit variables left)

au_col_x: DATA 47, 71, 95, 119

    DIM #au_hb                  ' land price line shown
    CONST AU_SB = $8800         ' land: "bids start at" price shown on row 11
    CONST AU_HELD = $880E       ' FRAME AND 255 when our goods line was let go
                                ' (2 bytes of cart RAM: 16-bit RAM is full)
    DIM au_good, au_stage, au_new   ' au_kind, au_role(4), #au_base are in isr.bas
    DIM au_push                 ' our stick: 0 released, 1 up, 2 down (as sent)
    DIM au_ln1                  ' pixel row of the sellers' line (255 none)
    DIM au_sp_col(4)            ' sprite colour words (low bits)
    DIM au_show_t               ' per-good status: animation step shown
    DIM au_bar(5), au_bar_t(5)  ' status bars (4 players + store): now, target
    DIM au_msg                  ' message on row 11
    DIM au_blink                ' seats whose sprite blinks (bit per seat)

' ---------------------------------------------------------------------------
' Screen setup.
' ---------------------------------------------------------------------------
auction_enter: PROCEDURE
    anim = AN_NONE : au_on = 0
    GOSUB fx_stop_all
    GOSUB hide_mobs
    scr_bg = C_GREY : GOSUB screen_stack
    ' Each player's standing (front) frame and a walking frame.
    FOR k = 0 TO 3
        j = pl8(k, S_PLAYER_SPECIES) AND 7
        g_slot = G_AU_SP + k * 4 : g_n = 4 : #g_src = VARPTR gfx_species((j * 11 + 5) * 16) : GOSUB gram_load
        g_slot = G_AU_SP2 + k * 4 : g_n = 4 : #g_src = VARPTR gfx_species((j * 11 + 6) * 16) : GOSUB gram_load
        c = pl_mobcol(pl8(k, S_PLAYER_COLOR) AND 3)
        au_sp_col(k) = (c AND 7) + (c AND 8) * 2       ' bit 3 -> bit 4 (x $200 later)
    NEXT k
    g_slot = G_AU_HLINE : g_n = 8 : #g_src = VARPTR gfx_hline(0) : GOSUB gram_load
    g_slot = G_AU_SBAR : g_n = 8 : #g_src = VARPTR gfx_sbar(0) : GOSUB gram_load
    g_slot = G_AU_ICON : g_n = 1 : #g_src = VARPTR gfx_storeicon(0) : GOSUB gram_load
    g_slot = G_AU_FLAG : g_n = 1 : #g_src = VARPTR gfx_flag(0) : GOSUB gram_load
    g_slot = G_BAR : g_n = 8 : #g_src = VARPTR gfx_bar(0) : GOSUB gram_load
    scr_now = SCR_AUCTION
    au_ln1 = 255
    au_show_t = 255 : au_msg = 255 : #au_hb = 65535
    bar_last = 255
END

' The title: "AUCTION #n" / "STATUS #n" over the good, or the land title.
auction_title: PROCEDURE
    t_pos = 0 : GOSUB clear_row
    t_pos = 20 : GOSUB clear_row
    IF au_kind = 1 THEN
        #val = round : #lit_addr = VARPTR t_titles(0) : lit_n = T_LANDAUCTION : GOSUB lit_nth
        #t_src = #lit_addr : GOSUB text_expand
        t_pos = (20 - t_len) / 2 : #t_col = C_RED : #t_src = TXT : GOSUB put_ram
        RETURN
    END IF
    #lit_addr = VARPTR t_words(0)
    IF au_stage = STAGE_SHOW OR au_stage = STAGE_DECLARE THEN lit_n = 8 ELSE lit_n = 7
    GOSUB lit_nth
    #t_src = #lit_addr : #val = round : GOSUB text_expand
    POKE TXT + t_len, 48 + round / 10 : IF round >= 10 THEN t_len = t_len + 1
    POKE TXT + t_len, 48 + round % 10 : t_len = t_len + 1
    POKE TXT + t_len, 0
    t_pos = (20 - t_len) / 2 : #t_col = C_RED : t_len = 20 : #t_src = TXT : GOSUB put_ram
    #lit_addr = VARPTR t_goods(0) : lit_n = au_good : GOSUB lit_nth
    #t_src = #lit_addr : t_pos = 20 : #t_col = C_BLUE : GOSUB put_lstr_c
END

' ---------------------------------------------------------------------------
' auction_state: a new state arrived in phases 2, 5 or 6.
' ---------------------------------------------------------------------------
auction_state: PROCEDURE
    au_kind = st8(ST_AUCTION_KIND)
    au_new = st8(ST_AUCTION_STAGE)
    au_good = st8(ST_AUCTION_GOOD) AND 3
    au_stepv = st8(ST_AUCTION_STEP)
    IF au_stepv = 0 THEN au_stepv = 1
    FOR i = 0 TO 3
        au_role(i) = st8(ST_AUCTION_ROLE + i)
    NEXT i

    ' The land auction starts with the plot blinking on the map.
    IF au_kind = 1 AND au_new = STAGE_SHOW THEN
        IF scr_now <> SCR_MAP OR au_stage <> STAGE_SHOW THEN
            IF scr_now <> SCR_MAP THEN GOSUB map_enter
            #val = round : title_id = T_LANDAUCTION : GOSUB title_show
            #val = st16(ST_AUCTION_START)
            msg_row = ROW_MSG : #msg_col = C_BLACK : msg_id = M_BIDSTART : GOSUB msg_show
        END IF
        au_stage = au_new
        map_blink = st8(ST_AUCTION_PLOT)
        RETURN
    END IF
    map_blink = 255

    IF scr_now <> SCR_AUCTION OR au_new <> au_stage THEN
        IF scr_now <> SCR_AUCTION THEN GOSUB auction_enter
        au_stage = au_new
        GOSUB auction_title
        GOSUB au_stage_begin
    END IF

    ' Where the lines are drawn from. In a land auction the scale scrolls
    ' up with the bidding (isr_auction), so the base only resets for a new
    ' plot.
    IF au_kind = 1 THEN
        IF au_stage < STAGE_ACTIVE OR #au_base < st16(ST_AUCTION_START) THEN #au_base = st16(ST_AUCTION_START)
    ELSE
        #au_base = st16(ST_AUCTION_STORE_BUY)
    END IF
    GOSUB au_snap
    IF au_stage >= STAGE_LEAD THEN
        au_on = 1 : anim = AN_AUCTION
    END IF
    GOSUB au_money
    GOSUB au_lines
    GOSUB au_timebar
    GOSUB au_message
    GOSUB au_land_line
    GOSUB au_blinkers
    IF st8(ST_AUCTION_TRADED) <> au_traded THEN
        IF st8(ST_AUCTION_TRADED) > au_traded THEN GOSUB snd_trade
        au_traded = st8(ST_AUCTION_TRADED)
    END IF
END

' What to draw when a stage starts.
au_stage_begin: PROCEDURE
    FOR i = 2 TO 11
        t_pos = i * 20 : GOSUB clear_row
    NEXT i
    au_ln1 = 255 : POKE AU_LN2, 255 : au_msg = 255 : bar_last = 255 : POKE AU_SB, 255 : POKE AU_SB + 1, 255
    ' A new plot or good: the server has no push for us yet.
    IF au_stage = STAGE_SHOW THEN au_push = 0
    IF au_kind = 2 AND au_stage = STAGE_SHOW THEN
        ' Per-good status: bars grow from the previous amount (au_status_tick).
        au_on = 0 : anim = AN_NONE
        GOSUB au_status_setup
        RETURN
    END IF
    GOSUB snd_au_stage
    ' Players: sellers at the top, buyers at the bottom (au_draw).
    GOSUB au_labels
END

' Row 9, over each player's money: "BUY" / "SELL" while the player stands at
' its starting line, else the price it has moved to (as on the Atari). And
' for goods the store's icons with its prices: selling above the cyan one,
' buying under the brown one.
au_labels: PROCEDURE
    FOR i = 0 TO 7 : POKE AU_LBL + i, 255 : NEXT i     ' redraw every field
    GOSUB au_lbl_tick
    IF au_kind = 2 THEN
        #BACKTAB(2 * 20 + 17) = gram_word(G_AU_ICON, C_CYAN)
        #BACKTAB(7 * 20 + 1) = gram_word(G_AU_ICON, C_BROWN)
        #t_col = C_BLACK
        #val = st16(ST_AUCTION_STORE_SELL) : t_pos = 1 * 20 + 14 : t_len = 4 : GOSUB put_numr
        #val = st16(ST_AUCTION_STORE_BUY) : t_pos = 8 * 20 : t_len = 4 : GOSUB put_numr
    END IF
END

' au_lbl_tick (every frame): redraw the row 9 fields that changed. AU_LBL
' holds what each shows: 0 the label, 1 nothing, else a price.
au_lbl_tick: PROCEDURE
    FOR i = 0 TO 3
        IF au_role(i) = ROLE_NONE THEN
            #w = 1
        ELSEIF au_stage < STAGE_LEAD THEN
            #w = 0
        ELSE
            #w = #au_pos(i)
            IF au_role(i) = ROLE_BUYER AND #w <= st16(ST_AUCTION_LO) THEN #w = 0
            IF au_role(i) = ROLE_SELLER AND #w >= st16(ST_AUCTION_HI) THEN #w = 0
            IF #w = 1 THEN #w = 2
        END IF
        IF #w <> (PEEK(AU_LBL + i * 2) AND 255) + (PEEK(AU_LBL + i * 2 + 1) AND 255) * 256 THEN
            POKE AU_LBL + i * 2, #w AND 255 : POKE AU_LBL + i * 2 + 1, #w / 256
            t_pos = 9 * 20 + i * 5
            FOR k = 0 TO 4 : #BACKTAB(t_pos + k) = 0 : NEXT k
            #t_col = pl_txtcol(pl8(i, S_PLAYER_COLOR) AND 3)
            IF #w = 0 THEN
                #lit_addr = VARPTR t_words(0)
                IF au_role(i) = ROLE_SELLER THEN lit_n = 9 ELSE lit_n = 10
                GOSUB lit_nth
                t_len = PEEK(#lit_addr) : #t_src = #lit_addr + 1
                t_pos = t_pos + 5 - t_len : GOSUB put_text
            ELSEIF #w > 1 THEN
                #val = #w : t_len = 5 : GOSUB put_numr
            END IF
        END IF
    NEXT i
END

' Money on row 10 and (goods, no message showing) units on row 11, in
' five-column fields: 4 players x 5 = the full width.
au_money: PROCEDURE
    FOR i = 0 TO 3
        #t_col = pl_txtcol(pl8(i, S_PLAYER_COLOR) AND 3)
        #val = pl16(i, S_PLAYER_MONEY)
        t_pos = 10 * 20 + i * 5 : t_len = 5 : GOSUB put_numr
        IF au_kind = 2 AND au_stage >= STAGE_LEAD AND au_msg = 255 THEN
            #val = pl8(i, S_PLAYER_GOODS + au_good)
            t_pos = 11 * 20 + i * 5 : t_len = 4 : GOSUB put_numr
        END IF
    NEXT i
END

' The dotted price lines: the store's sell/buy lines and, while trading,
' the highest buyer's and lowest seller's lines.
au_lines: PROCEDURE
    IF au_stage = STAGE_SHOW OR (au_kind = 1 AND au_stage < STAGE_LEAD) THEN RETURN
    ' The sellers' line: the lowest asking price, the store's while it has
    ' stock (goods), else none.
    #w = 32767
    IF au_kind = 2 AND st8(ST_STORE_STOCK + au_good) > 0 THEN #w = st16(ST_AUCTION_STORE_SELL)
    FOR i = 0 TO 3
        IF au_role(i) = ROLE_SELLER AND au_stage >= STAGE_LEAD AND #au_pos(i) < #w THEN #w = #au_pos(i)
    NEXT i
    j = 255
    IF #w < 32767 THEN GOSUB au_price_y : j = y
    ' The buyers' line: the highest bid, at least the store's buying price
    ' (goods) or the bottom of the scale (land).
    IF au_kind = 2 THEN #w = st16(ST_AUCTION_STORE_BUY) ELSE #w = #au_base
    FOR i = 0 TO 3
        IF au_role(i) = ROLE_BUYER AND au_stage >= STAGE_LEAD AND #au_pos(i) > #w THEN #w = #au_pos(i)
    NEXT i
    GOSUB au_price_y
    ' Never below row 7: row 9 holds the BUY / SEL labels.
    IF j > 63 THEN j = 255
    IF y > 63 THEN y = 255
    IF j = au_ln1 AND y = (PEEK(AU_LN2) AND 255) THEN RETURN
    ' Moved: clear the old rows, draw the new.
    IF au_ln1 < 255 THEN k = au_ln1 / 8 : GOSUB au_hline_clr
    k = PEEK(AU_LN2) AND 255
    IF k < 255 THEN k = k / 8 : GOSUB au_hline_clr
    au_ln1 = j : POKE AU_LN2, y
    IF j < 255 AND y < 255 AND j / 8 = y / 8 AND j <> y THEN
        DEFINE G_AU_HL2, 1, VARPTR gfx_hline2(((j AND 7) * 8 + (y AND 7)) * 4)
        #w = gram_word(G_AU_HL2, C_BLACK) : k = j : GOSUB au_hline_w
    ELSE
        IF j < 255 THEN k = j : GOSUB au_hline_at
        IF y < 255 THEN k = y : GOSUB au_hline_at
    END IF
END

' au_hline_clr: blank card row k between the store's icons.
au_hline_clr: PROCEDURE
    #addr = k * 20
    FOR i = 2 TO 16 : #BACKTAB(#addr + i) = 0 : NEXT i
END

' au_price_y: #w (price) -> y (pixel row of the line).
au_price_y: PROCEDURE
    IF #w >= #au_base THEN
        #addr = #w - #au_base
        IF #addr > (AU_Y_BASE - AU_Y_TOP) * 4 THEN #addr = (AU_Y_BASE - AU_Y_TOP) * 4
        IF au_stepv = 4 THEN #addr = #addr / 4
        IF #addr > AU_Y_BASE - AU_Y_TOP THEN y = AU_Y_TOP ELSE y = AU_Y_BASE - #addr
    ELSE
        #addr = #au_base - #w
        IF #addr > (AU_Y_BOT - AU_Y_BASE) * 4 THEN #addr = (AU_Y_BOT - AU_Y_BASE) * 4
        IF au_stepv = 4 THEN #addr = #addr / 4
        IF #addr > AU_Y_BOT - AU_Y_BASE THEN y = AU_Y_BOT ELSE y = AU_Y_BASE + #addr
    END IF
END

' Draw a dotted line across columns 2..16 at pixel row k.
au_hline_at: PROCEDURE
    #w = gram_word(G_AU_HLINE + (k AND 7), C_BLACK)
    GOSUB au_hline_w
END

' au_hline_w: card word #w across card row k / 8.
au_hline_w: PROCEDURE
    #addr = (k / 8) * 20
    FOR i = 2 TO 16
        #BACKTAB(#addr + i) = #w
    NEXT i
END

' The auction clock: a bar in column 18, one pixel per two ticks.
au_timebar: PROCEDURE
    bar_col = 18 : bar_rows = 8 : bar_colr = C_BLACK
    IF au_stage = STAGE_ACTIVE OR au_stage = STAGE_DECLARE THEN
        bar_px = st8(ST_AUCTION_TICKS) / 2
    ELSE
        bar_px = 0
    END IF
    GOSUB bar_draw
END

' Row 11 messages.
au_message: PROCEDURE
    k = 255
    IF au_stage = STAGE_DECLARE AND my_seat < 4 THEN k = M_DECLARE
    IF au_stage = STAGE_DONE THEN k = M_AUCOVER
    IF au_kind = 1 AND au_stage = STAGE_DONE AND st8(ST_AUCTION_WINNER) < 4 THEN k = M_SOLD
    IF au_kind = 1 AND au_stage = STAGE_DONE AND st8(ST_AUCTION_WINNER) > 3 THEN k = M_NOBIDS
    IF au_kind = 2 AND au_stage = STAGE_ACTIVE AND st8(ST_AUCTION_FLAGS) AND AF_COLLUDING THEN k = M_COLLUDE
    IF au_kind = 2 AND au_stage = STAGE_ACTIVE AND st8(ST_AUCTION_FLAGS) AND AF_CRITICAL THEN k = M_CRITICAL
    ' Land: the bottom of the (scrolling) scale, in green, as on the Atari.
    IF au_kind = 1 AND au_stage = STAGE_ACTIVE THEN
        #w = (PEEK(AU_SB) AND 255) + (PEEK(AU_SB + 1) AND 255) * 256
        IF k <> au_msg OR #au_base <> #w THEN
            au_msg = k : POKE AU_SB, #au_base AND 255 : POKE AU_SB + 1, #au_base / 256 : #val = #au_base
            msg_row = 220 : #msg_col = C_DKGREEN : msg_id = M_BIDSTART : GOSUB msg_show
        END IF
        RETURN
    END IF
    IF k = au_msg THEN RETURN
    au_msg = k
    IF k = 255 THEN t_pos = 220 : GOSUB clear_row : RETURN
    msg_row = 220 : #msg_col = C_RED : msg_id = k : GOSUB msg_show
END

' The land auction's price line (row 1): the opening price, then the high
' bid, then what it sold for.
au_land_line: PROCEDURE
    IF au_kind <> 1 THEN RETURN
    #val = st16(ST_AUCTION_START) : msg_id = M_BIDSTART
    FOR i = 0 TO 3
        IF au_role(i) = ROLE_BUYER AND #au_pos(i) >= #val THEN #val = #au_pos(i) : msg_id = M_HIGHBID
    NEXT i
    IF au_stage = STAGE_DONE AND st16(ST_AUCTION_PRICE) THEN #val = st16(ST_AUCTION_PRICE) : msg_id = M_SALEPRICE
    IF #val = #au_hb THEN RETURN
    #au_hb = #val
    k = msg_row : msg_row = 20 : #msg_col = C_BLUE : GOSUB msg_show
    msg_row = k
END

' au_blinkers: who is dealing blinks, as on the Atari: in a land auction the
' high bidder (the winner once sold); in a goods auction the buyer and seller
' whose lines meet, or whoever meets the store's line.
au_blinkers: PROCEDURE
    au_blink = 0
    IF au_stage < STAGE_ACTIVE THEN RETURN
    IF au_kind = 1 THEN
        IF au_stage = STAGE_DONE THEN
            j = st8(ST_AUCTION_WINNER)
            IF j < 4 THEN au_blink = au_bit(j)
            RETURN
        END IF
        #w = st16(ST_AUCTION_START) : j = 255
        FOR i = 0 TO 3
            IF au_role(i) = ROLE_BUYER AND #au_pos(i) >= #w THEN #w = #au_pos(i) : j = i
        NEXT i
        IF j < 4 THEN au_blink = au_bit(j)
        RETURN
    END IF
    IF au_stage <> STAGE_ACTIVE THEN RETURN
    ' Highest buyer, lowest seller.
    #w = 0 : j = 255 : #addr = 32767 : k = 255     ' (comparisons are signed)
    FOR i = 0 TO 3
        IF au_role(i) = ROLE_BUYER AND #au_pos(i) >= #w THEN #w = #au_pos(i) : j = i
        IF au_role(i) = ROLE_SELLER AND #au_pos(i) <= #addr THEN #addr = #au_pos(i) : k = i
    NEXT i
    IF j < 4 AND k < 4 THEN
        IF #w >= #addr THEN au_blink = au_bit(j) + au_bit(k) : RETURN
    END IF
    IF j < 4 THEN
        IF #w >= st16(ST_AUCTION_STORE_SELL) THEN au_blink = au_bit(j)
    END IF
    IF k < 4 THEN
        IF #addr <= st16(ST_AUCTION_STORE_BUY) THEN au_blink = au_blink + au_bit(k)
    END IF
END
au_bit: DATA 1, 2, 4, 8

' ---------------------------------------------------------------------------
' au_draw: place the players' sprites (called by the frame handler every 8
' frames, and by the main loop after a state arrives). Interrupt-safe: uses
' only the handler's scratch (ii, ix, iy, #iw).
' ---------------------------------------------------------------------------
au_draw: PROCEDURE
    FOR ii = 0 TO 3
        IF au_role(ii) = ROLE_NONE THEN
            SPRITE ii * 2, 0, 0, 0 : SPRITE ii * 2 + 1, 0, 0, 0
        ELSE
            IF au_stage < STAGE_LEAD THEN
                IF au_role(ii) = ROLE_SELLER THEN iy = AU_Y_SELL ELSE iy = AU_Y_BOT
            ELSE
                ' Price steps from the base: clamp first, then divide by the
                ' step ($1 or $4) with a shift. (IntyBASIC's division by a
                ' variable loops, and this runs in the frame handler.)
                #iw = #au_pos(ii)
                IF #iw >= #au_base THEN
                    #iw = #iw - #au_base
                    IF #iw > (AU_Y_BASE - AU_Y_TOP) * 4 THEN #iw = (AU_Y_BASE - AU_Y_TOP) * 4
                    IF au_stepv = 4 THEN #iw = #iw / 4
                    IF #iw > AU_Y_BASE - AU_Y_TOP THEN iy = AU_Y_TOP ELSE iy = AU_Y_BASE - #iw
                ELSE
                    #iw = #au_base - #iw
                    IF #iw > (AU_Y_BOT - AU_Y_BASE) * 4 THEN #iw = (AU_Y_BOT - AU_Y_BASE) * 4
                    IF au_stepv = 4 THEN #iw = #iw / 4
                    IF #iw > AU_Y_BOT - AU_Y_BASE THEN iy = AU_Y_BOT ELSE iy = AU_Y_BASE + #iw
                END IF
            END IF
            ' Feet on the line: the sprite is 11 pixels tall; MOB y is +8.
            iy = iy - 3
            ix = au_col_x(ii)
            #iw = $0800 + (G_AU_SP + ii * 4) * 8 + (au_sp_col(ii) AND 7) + (au_sp_col(ii) AND 16) * $100
            IF #au_pos(ii) <> #au_tgt(ii) AND (FRAME AND 8) THEN #iw = #iw + (G_AU_SP2 - G_AU_SP) * 8
            ' Dealing: 16 frames on, 16 off.
            IF (au_blink AND au_bit(ii)) <> 0 AND (FRAME AND 16) <> 0 THEN ix = 0
            IF ix THEN
                SPRITE ii * 2, ix + VISIBLE, iy + DOUBLEY, #iw
                SPRITE ii * 2 + 1, ix + VISIBLE, (iy + 8) + DOUBLEY, #iw + 16
            ELSE
                SPRITE ii * 2, 0, 0, 0 : SPRITE ii * 2 + 1, 0, 0, 0
            END IF
        END IF
    NEXT ii
END

' ---------------------------------------------------------------------------
' Per frame in phases 2, 5 and 6.
' ---------------------------------------------------------------------------
auction_tick: PROCEDURE
    GOSUB bar_tick_au
    IF au_kind = 1 AND au_stage = STAGE_SHOW THEN
        ' The plot for sale blinks on the map (a white outline, 16 frames on
        ' and 16 off); the button skips the wait.
        IF scr_now = SCR_MAP AND (FRAME AND 15) = 0 THEN
            mp = map_blink
            IF mp < 45 THEN
                map_blink_on = (FRAME / 16) AND 1
                GOSUB map_draw_plot
            END IF
        END IF
        GOSUB msg_tick_run
        IF my_seat < 4 THEN
            IF ev_btn THEN act = ACT_READY : poll_wait = 0
            GOSUB au_stick
        END IF
        RETURN
    END IF
    IF scr_now <> SCR_AUCTION THEN RETURN
    IF au_kind = 2 AND au_stage = STAGE_SHOW THEN GOSUB au_status_tick : RETURN
    GOSUB au_lines                  ' the price lines follow the players
    GOSUB au_lbl_tick               ' and the prices under them
    IF au_stage = STAGE_DECLARE AND my_seat < 4 THEN
        ' Push the stick to declare: up = seller, down = buyer.
        IF inp_dir AND 4 THEN
            IF au_role(my_seat) <> ROLE_SELLER THEN au_role(my_seat) = ROLE_SELLER : act = ACT_DECLARE : #act_arg = 2 : poll_wait = 0 : GOSUB au_labels : GOSUB snd_select
        ELSEIF inp_dir AND 1 THEN
            IF au_role(my_seat) <> ROLE_BUYER THEN au_role(my_seat) = ROLE_BUYER : act = ACT_DECLARE : #act_arg = 1 : poll_wait = 0 : GOSUB au_labels : GOSUB snd_select
        END IF
        IF ev_key = 0 THEN au_role(my_seat) = ROLE_NONE : act = ACT_DECLARE : #act_arg = 0 : poll_wait = 0 : GOSUB au_labels
        GOSUB au_draw
        RETURN
    END IF
    IF (au_stage = STAGE_ACTIVE OR au_stage = STAGE_LEAD) AND my_seat < 4 THEN
        GOSUB au_stick
        IF ev_btn AND au_kind = 2 AND au_stage = STAGE_ACTIVE THEN act = ACT_COLLUDE : poll_wait = 0
    END IF
    ' The clock bar runs between polls.
END

' au_stick: tell the server when our stick changes (up, down, released).
' It is sent before the lines are released too (the plot for sale, the lead
' into trading): the server holds it, so we start moving with the computers.
' The push carries au_traded, so the server can give back units that traded
' after what we have shown when we back off (net.bas compose).
au_stick: PROCEDURE
    k = 0
    IF inp_dir AND 4 THEN k = 1
    IF inp_dir AND 1 THEN k = 2
    IF k = au_push OR act <> ACT_NONE THEN RETURN
    au_push = k : act = ACT_PUSH : #act_arg = k : poll_wait = 0
    ' A goods line let go stays where we saw it stop (the server's is behind).
    IF k = 0 AND au_kind = 2 AND au_stage = STAGE_ACTIVE THEN act = ACT_HOLD : #act_arg = #au_pos(my_seat) : POKE AU_HELD, FRAME AND 255
    IF au_stage <> STAGE_ACTIVE THEN RETURN
    ' Move our own line straight away; the server confirms.
    IF k = 1 THEN
        ' Land: as high as our money goes (the server's limit for a land
        ' buyer; the scale scrolls up with it); goods: the top of the range.
        IF au_kind = 1 THEN
            #w = pl16(my_seat, S_PLAYER_MONEY)
            #au_tgt(my_seat) = #w - (#w AND 3)
        ELSE
            #au_tgt(my_seat) = st16(ST_AUCTION_HI)
        END IF
    END IF
    IF k = 2 THEN #au_tgt(my_seat) = st16(ST_AUCTION_LO)
    IF k = 0 THEN #au_tgt(my_seat) = #au_pos(my_seat)
END

' ---------------------------------------------------------------------------
' Per-good status (stage 0, 561 jiffies): previous amount, usage, spoilage,
' production, then what the store holds. Each player's bar stands over them.
' ---------------------------------------------------------------------------
au_status_setup: PROCEDURE
    ' Show the players standing at the bottom.
    FOR i = 0 TO 3 : au_role(i) = ROLE_BUYER : NEXT i
    au_stage = STAGE_SHOW
    GOSUB au_draw
    FOR i = 0 TO 3 : au_role(i) = st8(ST_AUCTION_ROLE + i) : NEXT i
    au_show_t = 255
    FOR i = 0 TO 4 : au_bar(i) = 0 : au_bar_t(i) = 0 : NEXT i
END

    DIM au_step_now
au_status_tick: PROCEDURE
    ' Which of the five steps are we in?
    #w = pe_now
    IF #w < 127 THEN
        k = 0
    ELSEIF #w < 250 THEN
        k = 1
    ELSEIF #w < 373 THEN
        k = 2
    ELSEIF #w < 496 THEN
        k = 3
    ELSE
        k = 4
    END IF
    IF k <> au_show_t THEN
        au_show_t = k
        GOSUB au_status_targets
        msg_row = 220 : #msg_col = C_BLUE
        IF k < 4 THEN
            msg_id = M_PREVIOUS + k : GOSUB msg_show
        ELSE
            #val = st8(ST_STORE_STOCK + au_good)
            IF #val THEN msg_id = M_STOREHAS ELSE msg_id = M_STORENO
            GOSUB msg_show
            GOSUB au_need_draw
        END IF
    END IF
    ' Bars move one pixel every other frame.
    IF FRAME AND 1 THEN RETURN
    FOR i = 0 TO 3
        IF au_bar(i) < au_bar_t(i) THEN au_bar(i) = au_bar(i) + 1 : GOSUB au_bar_draw
        IF au_bar(i) > au_bar_t(i) THEN au_bar(i) = au_bar(i) - 1 : GOSUB au_bar_draw
    NEXT i
END

' Bar targets for step au_show_t, worked back from the current amount:
' previous = now - production + spoilage + usage.
au_status_targets: PROCEDURE
    FOR i = 0 TO 3
        #w = pl8(i, S_PLAYER_GOODS + au_good)
        c = pl8(i, S_PLAYER_PROD + au_good)
        j = pl8(i, S_PLAYER_SPOIL + au_good)
        h_k = round : GOSUB au_need             ' this round's usage

        #addr = #w + j + n
        IF #addr >= c THEN #addr = #addr - c ELSE #addr = 0      ' previous amount
        IF au_show_t = 0 THEN #addr = #addr
        IF au_show_t = 1 THEN
            IF #addr >= n THEN #addr = #addr - n ELSE #addr = 0
        END IF
        IF au_show_t = 2 THEN #addr = #w - c
        IF au_show_t >= 3 THEN #addr = #w
        ' Two pixels a unit, at most the five rows over the player.
        #addr = #addr * 2
        IF #addr > 40 THEN #addr = 40
        au_bar_t(i) = #addr
    NEXT i
END

' au_need: n = the units of au_good player i needs in round h_k: food by
' round (none after round 12), energy one per other M.U.L.E.
au_need: PROCEDURE
    n = 0
    IF au_good = GOOD_FOOD THEN
        IF h_k <= 12 THEN n = 3
        IF h_k >= 4 AND h_k <= 12 THEN n = 4
        IF h_k >= 8 AND h_k <= 12 THEN n = 5
    ELSEIF au_good = GOOD_ENERGY THEN
        FOR mp = 0 TO 44
            IF st8(ST_MAP_OWNER + mp) = i THEN
                IF st8(ST_MAP_MULE + mp) < 4 AND st8(ST_MAP_MULE + mp) <> GOOD_ENERGY THEN n = n + 1
            END IF
        NEXT mp
    END IF
END

' Draw bar i (au_bar(i) pixels) in its player's column, over the player's
' head (rows 2..6: five rows, 20 units).
au_bar_draw: PROCEDURE
    c = pl_mobcol(pl8(i, S_PLAYER_COLOR) AND 3)
    j = au_bar(i)
    x = 5 + i * 3
    FOR k = 1 TO 5
        #addr = (7 - k) * 20 + x            ' rows 6 up to 2, clear of the heads
        IF j >= 8 THEN
            #BACKTAB(#addr) = gram_word(G_AU_SBAR + 7, c) : j = j - 8
        ELSEIF j > 0 THEN
            #BACKTAB(#addr) = gram_word(G_AU_SBAR + j - 1, c) : j = 0
        ELSE
            #BACKTAB(#addr) = 0
        END IF
    NEXT k
END
