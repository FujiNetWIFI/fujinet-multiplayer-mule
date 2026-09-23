' ===========================================================================
' fx.bas -- scripted sprite animations, stepped by the frame handler.
'
' The Atari plays set pieces between phases: the transport ship landing and
' leaving, the pirate ship, the pest crawling round its plot, the acid rain
' cloud, the players walking into the status summary, and so on. Each one
' is a little script in DATA (events.bas, status.bas) run by one of four
' actors. The frame handler steps every actor once per frame, so the pieces
' keep the Atari's speeds even while the main loop is stuck on the network.
'
' Actor state lives in cartridge RAM (IntyBASIC's own variables are far too
' few): field f of actor a is at FXB + f + a. Positions are stored biased
' (x + 64, y + 32) in 8 bits so sprites can start or finish off screen.
'
' Script words:
'   FX_POS x, y              jump to x, y (MOB coordinates)
'   FX_MOVE dx, dy, per, n   n steps of (dx, dy), one every per frames
'   FX_WAIT n                n frames
'   FX_LOOK kind, mob, f, d2, xattr, yattr
'                            kind FK_*; f = SPRITE f word (card + colour);
'                            d2 = cards from f to the second frame (walk
'                            cycle), or for FK_SHIP to the body's first card;
'                            xattr = X attribute / 256 (ZOOMX2 = 4) plus, for
'                            ships, the top colour * 256; yattr = Y attribute /
'                            128 (DOUBLEY 1, ZOOMY2 2, ZOOMY4 4) plus the
'                            ship's bottom colour * 256
'   FX_CARD f                change the card/colour word
'   FX_YATTR yattr           change the Y attribute
'   FX_HIDE                  take the actor's MOBs off the screen
'   FX_SIG v                 set fx_flag = v for the main loop
'   FX_LOOP n, back          repeat the `back` words before this n more times
'   FX_END                   stop (the MOBs stay as they are)
' ===========================================================================

    CONST FX_END = 0
    CONST FX_POS = 1
    CONST FX_MOVE = 2
    CONST FX_WAIT = 3
    CONST FX_LOOK = 4
    CONST FX_CARD = 5
    CONST FX_YATTR = 6
    CONST FX_HIDE = 7
    CONST FX_SIG = 8
    CONST FX_LOOP = 9

    CONST FK_SINGLE = 1         ' one MOB
    CONST FK_PAIR = 2           ' species: two 8x16 half-height MOBs (mob, mob+1)
    CONST FK_SHIP = 3           ' transport/pirate ship: nose = MOB 7, body = 0..3

    ' Actor fields (4 bytes each, one per actor).
    CONST FXB = $8600
    CONST AF_ST = 0             ' running
    CONST AF_X = 4
    CONST AF_Y = 8
    CONST AF_WAIT = 12
    CONST AF_N = 16             ' move steps left
    CONST AF_DX = 20
    CONST AF_DY = 24
    CONST AF_PER = 28
    CONST AF_KIND = 32
    CONST AF_MOB = 36
    CONST AF_XA = 40
    CONST AF_YA = 44
    CONST AF_ALT = 48           ' walk frame toggle
    CONST AF_REP = 52           ' loop counter
    CONST AF_C1 = 56            ' ship colours / summary colour
    CONST AF_C2 = 60
    CONST AF_D2 = 64            ' second frame, in cards from f

    ' fx(field) of actor fa (main loop), fxi(field) of actor ia (frame handler).
    DEF FN fx(f) = (PEEK(FXB + (f) + fa) AND 255)
    DEF FN fxi(f) = (PEEK(FXB + (f) + ia) AND 255)

' fx_start: run script #addr on actor fa. Set AF_X/AF_Y first if the script
' doesn't. (Main loop only.)
fx_start: PROCEDURE
    POKE FXB + AF_ST + fa, 0
    #fx_pc(fa) = #addr
    POKE FXB + AF_WAIT + fa, 1 : POKE FXB + AF_N + fa, 0
    POKE FXB + AF_REP + fa, 0 : POKE FXB + AF_ALT + fa, 0
    POKE FXB + AF_ST + fa, 1
    fx_on = 1
END

' fx_stop_all: stop every actor and hide its sprites. (Main loop only.)
fx_stop_all: PROCEDURE
    fx_on = 0
    FOR fa = 0 TO 3
        IF fx(AF_ST) THEN
            k = fx(AF_MOB)
            IF fx(AF_KIND) = FK_SHIP THEN
                SPRITE 0, 0, 0, 0 : SPRITE 1, 0, 0, 0 : SPRITE 2, 0, 0, 0 : SPRITE 3, 0, 0, 0 : SPRITE 7, 0, 0, 0
            ELSE
                SPRITE k, 0, 0, 0
                IF fx(AF_KIND) = FK_PAIR THEN SPRITE k + 1, 0, 0, 0
            END IF
        END IF
        POKE FXB + AF_ST + fa, 0
    NEXT fa
END

' fx_clear: forget all actors (at boot).
fx_clear: PROCEDURE
    FOR fa = 0 TO 67 : POKE FXB + fa, 0 : NEXT fa
    fx_on = 0
END

' ---------------------------------------------------------------------------
' Frame handler side (its own actor index, ia).
' ---------------------------------------------------------------------------
fx_run: PROCEDURE
    FOR ia = 0 TO 3
        IF fxi(AF_ST) THEN GOSUB fx_step
    NEXT ia
END

fx_step: PROCEDURE
    ic = fxi(AF_WAIT)
    IF ic > 1 THEN POKE FXB + AF_WAIT + ia, ic - 1 : RETURN
    ic = fxi(AF_N)
    IF ic THEN
        POKE FXB + AF_X + ia, fxi(AF_X) + fxi(AF_DX)
        POKE FXB + AF_Y + ia, fxi(AF_Y) + fxi(AF_DY)
        POKE FXB + AF_N + ia, ic - 1
        POKE FXB + AF_WAIT + ia, fxi(AF_PER)
        POKE FXB + AF_ALT + ia, fxi(AF_ALT) XOR 1
        GOSUB fx_draw
        RETURN
    END IF
    ' Fetch words until one of them takes time.
    POKE FXB + AF_WAIT + ia, 1
    WHILE 1
        fop = PEEK(#fx_pc(ia))
        #fx_pc(ia) = #fx_pc(ia) + 1
        IF fop = FX_END THEN POKE FXB + AF_ST + ia, 0 : RETURN
        IF fop = FX_POS THEN
            POKE FXB + AF_X + ia, PEEK(#fx_pc(ia)) + 64
            POKE FXB + AF_Y + ia, PEEK(#fx_pc(ia) + 1) + 32
            #fx_pc(ia) = #fx_pc(ia) + 2
            GOSUB fx_draw
        ELSEIF fop = FX_MOVE THEN
            POKE FXB + AF_DX + ia, PEEK(#fx_pc(ia))
            POKE FXB + AF_DY + ia, PEEK(#fx_pc(ia) + 1)
            POKE FXB + AF_PER + ia, PEEK(#fx_pc(ia) + 2)
            POKE FXB + AF_N + ia, PEEK(#fx_pc(ia) + 3)
            POKE FXB + AF_WAIT + ia, PEEK(#fx_pc(ia) + 2)
            #fx_pc(ia) = #fx_pc(ia) + 4
            RETURN
        ELSEIF fop = FX_WAIT THEN
            POKE FXB + AF_WAIT + ia, PEEK(#fx_pc(ia))
            #fx_pc(ia) = #fx_pc(ia) + 1
            RETURN
        ELSEIF fop = FX_LOOK THEN
            POKE FXB + AF_KIND + ia, PEEK(#fx_pc(ia))
            POKE FXB + AF_MOB + ia, PEEK(#fx_pc(ia) + 1)
            #fx_f(ia) = PEEK(#fx_pc(ia) + 2)
            POKE FXB + AF_D2 + ia, PEEK(#fx_pc(ia) + 3)
            #iw = PEEK(#fx_pc(ia) + 4)
            POKE FXB + AF_XA + ia, #iw AND 255 : POKE FXB + AF_C1 + ia, #iw / 256
            #iw = PEEK(#fx_pc(ia) + 5)
            POKE FXB + AF_YA + ia, #iw AND 255 : POKE FXB + AF_C2 + ia, #iw / 256
            #fx_pc(ia) = #fx_pc(ia) + 6
        ELSEIF fop = FX_CARD THEN
            #fx_f(ia) = PEEK(#fx_pc(ia))
            #fx_pc(ia) = #fx_pc(ia) + 1
            GOSUB fx_draw
        ELSEIF fop = FX_YATTR THEN
            POKE FXB + AF_YA + ia, PEEK(#fx_pc(ia))
            #fx_pc(ia) = #fx_pc(ia) + 1
            GOSUB fx_draw
        ELSEIF fop = FX_HIDE THEN
            GOSUB fx_hide
        ELSEIF fop = FX_SIG THEN
            fx_flag = PEEK(#fx_pc(ia))
            #fx_pc(ia) = #fx_pc(ia) + 1
        ELSEIF fop = FX_LOOP THEN
            ic = fxi(AF_REP)
            IF ic = 0 THEN ic = PEEK(#fx_pc(ia)) + 1
            ic = ic - 1
            POKE FXB + AF_REP + ia, ic
            IF ic THEN
                #fx_pc(ia) = #fx_pc(ia) - 1 - PEEK(#fx_pc(ia) + 1)
            ELSE
                #fx_pc(ia) = #fx_pc(ia) + 2
            END IF
        END IF
    WEND
END

' fx_draw: write the actor's MOBs.
fx_draw: PROCEDURE
    ix = fxi(AF_X) : iy = fxi(AF_Y)
    fmx = ix - 64 : fmy = iy - 32
    IF fxi(AF_KIND) = FK_SHIP THEN GOSUB fx_draw_ship : RETURN
    IF ix < 64 OR ix >= 232 OR iy < 32 THEN GOSUB fx_hide : RETURN
    #iw = #fx_f(ia)
    IF fxi(AF_ALT) THEN #iw = #iw + fxi(AF_D2) * 8
    ic = fxi(AF_MOB)
    IF fxi(AF_KIND) = FK_PAIR THEN
        SPRITE ic, fmx + VISIBLE + fxi(AF_XA) * 256, fmy + DOUBLEY + fxi(AF_YA) * 128, #iw
        SPRITE ic + 1, fmx + VISIBLE + fxi(AF_XA) * 256, (fmy + 8) + DOUBLEY + fxi(AF_YA) * 128, #iw + 16
    ELSE
        SPRITE ic, fmx + VISIBLE + fxi(AF_XA) * 256, fmy + fxi(AF_YA) * 128, #iw
    END IF
END

' The ship: a nose (8 px, 10 lines) in front of a 32 x 16 px body drawn by
' four double-width MOBs, the top half light and the bottom half dark.
fx_draw_ship: PROCEDURE
    ' nose (MOB 7)
    IF ix >= 72 AND ix < 240 THEN
        SPRITE 7, (fmx - 8) + VISIBLE, (fmy + 5) + DOUBLEY, #fx_f(ia)
    ELSE
        SPRITE 7, 0, 0, 0
    END IF
    ' body: left half MOBs 0 (top) and 2 (bottom), right half 1 and 3
    #iw = (#fx_f(ia) AND $0FF8) + fxi(AF_D2) * 8
    ic = fxi(AF_C1)
    #iw = #iw + (ic AND 7) + (ic AND 8) * $200
    IF ix >= 64 AND ix < 232 THEN
        SPRITE 0, fmx + VISIBLE + ZOOMX2, fmy + ZOOMY4, #iw
    ELSE
        SPRITE 0, 0, 0, 0
    END IF
    IF ix < 216 THEN
        SPRITE 1, (fmx + 16) + VISIBLE + ZOOMX2, fmy + ZOOMY4, #iw + 8
    ELSE
        SPRITE 1, 0, 0, 0
    END IF
    #iw = (#fx_f(ia) AND $0FF8) + fxi(AF_D2) * 8 + 16
    ic = fxi(AF_C2)
    #iw = #iw + (ic AND 7) + (ic AND 8) * $200
    IF ix >= 64 AND ix < 232 THEN
        SPRITE 2, fmx + VISIBLE + ZOOMX2, (fmy + 8) + ZOOMY4, #iw
    ELSE
        SPRITE 2, 0, 0, 0
    END IF
    IF ix < 216 THEN
        SPRITE 3, (fmx + 16) + VISIBLE + ZOOMX2, (fmy + 8) + ZOOMY4, #iw + 8
    ELSE
        SPRITE 3, 0, 0, 0
    END IF
END

fx_hide: PROCEDURE
    IF fxi(AF_KIND) = FK_SHIP THEN
        SPRITE 0, 0, 0, 0 : SPRITE 1, 0, 0, 0 : SPRITE 2, 0, 0, 0 : SPRITE 3, 0, 0, 0 : SPRITE 7, 0, 0, 0
    ELSE
        ic = fxi(AF_MOB)
        SPRITE ic, 0, 0, 0
        IF fxi(AF_KIND) = FK_PAIR THEN SPRITE ic + 1, 0, 0, 0
    END IF
END
