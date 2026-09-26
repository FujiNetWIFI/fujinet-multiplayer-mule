' ===========================================================================
' auneed.bas -- the per-good status screen's marks for what each player
' needs next round, and snapping the lines to a new state (auction.bas;
' kept apart only to spread the ROM).
' ===========================================================================

' au_need_draw: once the store's stock is shown, what each player needs for
' the next round, as on the Atari: a dotted line at that height across the
' player's bar and, under the player, how far short (white, "-") or over
' (black, "+") they are. The Atari's light grey would vanish on this grey,
' so short is white. The line is drawn either side of the bar (a card has
' one colour).
au_need_draw: PROCEDURE
    FOR i = 0 TO 3
        h_k = round + 1 : GOSUB au_need
        IF n THEN
            #w = pl8(i, S_PLAYER_GOODS + au_good)
            IF #w >= n THEN
                #t_col = C_BLACK : #val = #w - n : c = 11     ' "+"
            ELSE
                #t_col = C_WHITE : #val = n - #w : c = 13     ' "-"
            END IF
            ' The line's pixel row: bars are 2 px a unit up from row 6's foot.
            IF n > 20 THEN y = 16 ELSE y = 56 - n * 2
            #addr = (y / 8) * 20 + 4 + i * 3
            #BACKTAB(#addr) = gram_word(G_AU_HLINE + (y AND 7), #t_col)
            #BACKTAB(#addr + 2) = gram_word(G_AU_HLINE + (y AND 7), #t_col)
            t_pos = 9 * 20 + 5 + i * 3 : t_len = 2 : GOSUB put_numr
            IF #val >= 10 THEN t_pos = t_pos - 1
            #BACKTAB(t_pos) = c * 8 + #t_col
        END IF
    NEXT i
END

' au_snap: the lines from a new state. The frame handler walks them on.
au_snap: PROCEDURE
    ' Our released goods line: the server's is behind what we showed and
    ' carries on to where we let go (ACT_HOLD), so ours stays put rather
    ' than jumping back until the server's comes to rest (or a second).
    k = 255
    IF au_kind = 2 AND au_stage = STAGE_ACTIVE AND au_push = 0 AND ((FRAME - PEEK(AU_HELD)) AND 255) < 60 THEN k = my_seat
    FOR i = 0 TO 3
        IF i = k THEN
            IF st8(ST_AUCTION_PUSH + i) <> 0 OR st16(ST_AUCTION_POS + i * 2) <> st16(ST_AUCTION_TARGET + i * 2) THEN GOTO au_snap_next
        END IF
        #au_tgt(i) = st16(ST_AUCTION_TARGET + i * 2)
        ' Snap to the server's position; the frame handler walks it on.
        #au_pos(i) = st16(ST_AUCTION_POS + i * 2)
au_snap_next:
    NEXT i
END
