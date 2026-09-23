' ===========================================================================
' screen.bas -- screen, text and GRAM helpers shared by every screen.
' ===========================================================================

    ' Player colour index (MulePlayer.color, 0..3) -> MOB/GRAM colour and
    ' GROM text colour (colour-stack GROM text is limited to colours 0..7).
    ' The Atari's are blue $84, purple $54, green $C4, orange $34.
pl_mobcol: DATA C_BLUE, C_PURPLE, C_GREEN, C_ORANGE
pl_txtcol: DATA C_BLUE, C_RED, C_GREEN, C_TAN

    DIM g_slot, g_n, g_k, #g_src    ' gram_load arguments
    DIM t_pos, t_len            ' text helpers
    DIM #t_col                  ' colour word (FG/BG mode adds background bits)
    DIM #t_src

' ---------------------------------------------------------------------------
' gram_load: copy g_n cards from ROM address #g_src into GRAM from g_slot on,
' 16 cards a frame. Blocks for (g_n + 15) / 16 frames.
' ---------------------------------------------------------------------------
gram_load: PROCEDURE
    WHILE g_n > 0
        IF g_n > 16 THEN g_k = 16 ELSE g_k = g_n
        DEFINE g_slot, g_k, VARPTR gfx_map(#g_src - VARPTR gfx_map(0))
        WAIT
        g_slot = g_slot + g_k
        g_n = g_n - g_k
        #g_src = #g_src + g_k * 4
    WEND
END

' ---------------------------------------------------------------------------
' Screen modes. The map, auction and status screens use the colour stack
' (GRAM cards may use all 16 colours); the town uses foreground/background.
' ---------------------------------------------------------------------------
    DIM scr_mode, scr_bg

screen_stack: PROCEDURE            ' scr_bg: the one background colour
    MODE 0, scr_bg, scr_bg, scr_bg, scr_bg
    BORDER scr_bg
    scr_mode = 0
    GOSUB clear_screen
    WAIT
END

screen_fgbg: PROCEDURE
    MODE 1
    BORDER scr_bg
    scr_mode = 1
    GOSUB clear_screen
    WAIT
END

' Blank the whole screen (in FG/BG mode the blank card takes scr_bg).
clear_screen: PROCEDURE
    IF scr_mode = 1 THEN #h_w = fgbg_bg(scr_bg) ELSE #h_w = 0
    FOR #h_a = 0 TO 239
        #BACKTAB(#h_a) = #h_w
    NEXT #h_a
END


' Hide every MOB.
hide_mobs: PROCEDURE
    FOR h_i = 0 TO 7
        SPRITE h_i, 0, 0, 0
    NEXT h_i
END

' ---------------------------------------------------------------------------
' Text. put_text prints t_len characters of a DATA string (ASCII - 32, as
' IntyBASIC stores them) from #t_src at t_pos in colour #t_col. put_ram
' prints a NUL-terminated ASCII string from cartridge RAM (t_len = maximum),
' folding lower case to upper case (the GROM has no lower case in FG/BG
' mode). In FG/BG mode #t_col may include background bits.
' ---------------------------------------------------------------------------
put_text: PROCEDURE
    FOR h_i = 0 TO t_len - 1
        h_c = PEEK(#t_src + h_i)
        #BACKTAB(t_pos + h_i) = h_c * 8 + #t_col
    NEXT h_i
END

' A length-prefixed DATA string at #t_src.
put_lstr: PROCEDURE
    t_len = PEEK(#t_src)
    #t_src = #t_src + 1
    IF t_len THEN GOSUB put_text
END

' Length-prefixed string centred on row t_pos / 20.
put_lstr_c: PROCEDURE
    t_len = PEEK(#t_src)
    t_pos = (t_pos / 20) * 20 + (20 - t_len) / 2
    #t_src = #t_src + 1
    IF t_len THEN GOSUB put_text
END

put_ram: PROCEDURE
    FOR h_i = 0 TO t_len - 1
        h_c = PEEK(#t_src + h_i) AND 255
        IF h_c = 0 THEN EXIT FOR
        IF h_c >= 97 AND h_c <= 122 THEN h_c = h_c - 32
        IF h_c < 32 OR h_c > 95 THEN h_c = 63
        #BACKTAB(t_pos + h_i) = (h_c - 32) * 8 + #t_col
    NEXT h_i
END

' Blank a row (in the current background).
clear_row: PROCEDURE              ' t_pos = row start
    IF scr_mode = 1 THEN #h_w = fgbg_bg(scr_bg) ELSE #h_w = 0
    FOR h_i = 0 TO 19
        #BACKTAB(t_pos + h_i) = #h_w
    NEXT h_i
END

' put_numr: #val right-aligned in t_len columns ending at t_pos + t_len - 1.
' (8-bit FOR loops can't count down to 0, so this counts up from the right.)
put_numr: PROCEDURE
    #h_w = #val
    FOR h_i = 1 TO t_len
        IF #h_w > 0 OR h_i = 1 THEN
            #BACKTAB(t_pos + t_len - h_i) = ((#h_w % 10) + 16) * 8 + #t_col
            #h_w = #h_w / 10
        ELSE
            #BACKTAB(t_pos + t_len - h_i) = #t_col
        END IF
    NEXT h_i
END

' put_numl: #val left-aligned at t_pos; t_len returns the digits used.
put_numl: PROCEDURE
    #h_w = 10000 : h_k = 0 : t_len = 0
    WHILE #h_w > 0
        h_c = (#val / #h_w) % 10
        IF h_c > 0 OR h_k OR #h_w = 1 THEN
            #BACKTAB(t_pos + t_len) = (h_c + 16) * 8 + #t_col
            t_len = t_len + 1
            h_k = 1
        END IF
        #h_w = #h_w / 10
    WEND
END


' put_num32: #val (low 16 bits) + #val2 (high 16 bits) right-aligned in
' t_len columns at t_pos. Good to 999,999 (a colony total).
put_num32: PROCEDURE
    IF #val2 = 0 THEN GOSUB put_numr : RETURN
    ' value = hi * 65536 + lo = thousands * 1000 + rest
    #h_a = #val2 * 536 + (#val % 1000)
    #val = #val2 * 65 + #val / 1000 + #h_a / 1000     ' thousands
    #h_a = #h_a % 1000
    t_len = t_len - 3 : GOSUB put_numr
    t_pos = t_pos + t_len
    #BACKTAB(t_pos) = (#h_a / 100 + 16) * 8 + #t_col
    #BACKTAB(t_pos + 1) = ((#h_a / 10) % 10 + 16) * 8 + #t_col
    #BACKTAB(t_pos + 2) = (#h_a % 10 + 16) * 8 + #t_col
END
