' ===========================================================================
' terrain.bas -- how long each of our map steps takes, from what the small
' figure is touching (the Atari's collision test at $5837). Called from the
' frame handler (isr.bas), so it uses only the handler's variables.
' ===========================================================================

' walk_terrain: how long the next map step waits. The Atari times a step by
' the collisions its figure made where it stands ($5837): touching a
' mountain (player 2) and no playfield makes it 21 frames; touching the
' river (player 3) makes it 14. Here the figure's rows are tested against
' the same bitmaps the map is drawn from: the mountain cards, the rest of
' the plot cards (the playfield) and the river texture.
' walk_terrain does one background row of the check per frame, in the
' frames after a step (isr_walk calls it while t_l < 4; a step sets t_l = 0
' and t_h = 0). After the fourth row, a mountain adds 15 frames to the wait
' and the river 8, so a step still takes 21 or 14 frames. Split up, and
' inline, because it runs in the frame handler, which must finish inside
' the frame and shares IntyBASIC's 24-word stack with the main loop.
' The figure's six lines cover four background rows, so a row's two lines
' are ORed into one mask (ic). For each of the (up to) two cards under the
' row, the mask is shifted to the card's pixels (t_k), then tested against
' the card's mountain pixels (t_m), its other pixels (t_p, the playfield),
' and the river MOB's texture where it crosses card columns 9 and 10.
walk_terrain: PROCEDURE
    ix = (w_x - 8) / 8              ' the card column the figure starts in
        #iw = PEEK(VARPTR gfx_mapsp(0) + (w_sp * 10 + w_face) * 4 + t_l)
        ic = (#iw OR (#iw / 256)) AND 255
        iy = w_y - 8 + t_l
        IF ic <> 0 AND iy < 88 THEN
            FOR ii = 0 TO 1
                ' The figure's pixels over this card: ic moved right by the
                ' figure's offset in the left card (w_x AND 7), or what spills
                ' into the right one.
                #iw = ic : ia = 7 - (w_x AND 7)
                WHILE ia : #iw = #iw + #iw : ia = ia - 1 : WEND
                IF ii = 0 THEN t_k = #iw / 128 ELSE t_k = (#iw + #iw) AND 255
                IF t_k <> 0 AND ix + ii < 20 THEN
                    t_m = 0 : t_p = 0
                    #iw = #BACKTAB((iy / 8) * 20 + ix + ii)
                    IF #iw AND $0800 THEN
                        ia = (#iw / 8) AND 63
                        IF ia < MC_RIVER THEN
                            #iw = PEEK(VARPTR gfx_map(0) + ia * 4 + (iy AND 7) / 2)
                            IF iy AND 1 THEN t_p = #iw / 256 ELSE t_p = #iw
                            IF ia < MC_TLU + 3 THEN
                                t_m = t_p : t_p = 0
                            ELSEIF ia > MC_TLO AND ia < MC_TLO + 4 THEN
                                ia = MC_TLU + ia - MC_TLO - 1
                                #iw = PEEK(VARPTR gfx_map(0) + ia * 4 + (iy AND 7) / 2)
                                IF iy AND 1 THEN t_m = #iw / 256 ELSE t_m = #iw
                                t_p = t_p AND (t_m XOR 255)
                            END IF
                        END IF
                    END IF
                    IF t_k AND t_m THEN t_h = t_h OR 1
                    IF t_k AND t_p THEN t_h = t_h OR 2
                    ' The river: a MOB at x 84 (screen x 76..83, so the right
                    ' half of card column 9 and the left half of 10) with a
                    ' 16-row texture, 2 background rows a row, from row 8.
                    IF ix + ii = 9 OR ix + ii = 10 THEN
                        ia = ((iy - 8) / 2) AND 15
                        #iw = PEEK(VARPTR gfx_map(0) + MC_RIVER * 4 + ia / 2)
                        IF ia AND 1 THEN ia = #iw / 256 ELSE ia = #iw
                        IF ix + ii = 9 THEN ia = ia / 16 ELSE ia = ia * 16
                        IF t_k AND ia THEN t_h = t_h OR 4
                    END IF
                END IF
            NEXT ii
        END IF
    t_l = t_l + 1
    IF t_l < 4 THEN RETURN
    IF t_h AND 1 THEN
        IF (t_h AND 2) = 0 THEN w_cnt = w_cnt + 15
    ELSEIF t_h AND 4 THEN
        w_cnt = w_cnt + 8
    END IF
END

