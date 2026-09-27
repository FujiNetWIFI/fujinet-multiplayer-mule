' ===========================================================================
' sound.bas -- short effects on PSG channel A, no music.
'
' An effect sets a tone and its length; the frame handler (snd_isr) counts it
' down and plays an optional second tone, so nothing here blocks the game.
' Tone values are 3579545 / 32 / Hz. snd_myturn is the one that matters: rung
' whenever the game wants your hand on the controller, it is the only effect
' with two notes, and the loudest.
' ===========================================================================

    DIM snd_t, snd_t2, #snd_f2

' Frame handler side.
snd_isr: PROCEDURE
    IF snd_t = 0 THEN RETURN
    snd_t = snd_t - 1
    IF snd_t = 0 THEN
        IF snd_t2 THEN
            SOUND 0, #snd_f2, 15
            snd_t = snd_t2 : snd_t2 = 0
        ELSE
            SOUND 0, 1, 0
        END IF
    END IF
END

' snd_play: #val = tone, k = frames; #snd_f2 / snd_t2 = optional second tone
' (only the alert has one: it plays loud).
snd_play: PROCEDURE
    snd_t = 0
    IF snd_t2 THEN SOUND 0, #val, 15 ELSE SOUND 0, #val, 10
    snd_t = k
END

snd_select: PROCEDURE        ' a blip (~870 Hz)
    #val = 129 : k = 3 : snd_t2 = 0 : GOSUB snd_play
END
snd_sample: PROCEDURE        ' a low dig
    #val = 405 : k = 3 : snd_t2 = 0 : GOSUB snd_play
END
snd_error: PROCEDURE
    #val = 783 : k = 8 : snd_t2 = 0 : GOSUB snd_play
END
snd_buy: PROCEDURE
    #val = 101 : k = 4 : snd_t2 = 0 : GOSUB snd_play
END
snd_myturn: PROCEDURE        ' the alert: your hand to the controller
    #val = 107 : k = 8 : #snd_f2 = 77 : snd_t2 = 14 : GOSUB snd_play
END
' The alert, for a seated player (the callers are in the full segment 2).
snd_alert: PROCEDURE
    IF my_seat < 4 THEN GOSUB snd_myturn
END
' An auction's stage begins: declaring, and the lines about to go, want
' your hand on the controller.
snd_au_stage: PROCEDURE
    IF au_stage = STAGE_DECLARE OR au_stage = STAGE_LEAD THEN GOSUB snd_alert
    IF au_stage = STAGE_DONE THEN GOSUB snd_auction_over
END
' The lobby's countdown has begun (#lb_lastcd 65534: nothing yet, 65535:
' waiting).
snd_lobby_cd: PROCEDURE
    IF (#lb_lastcd AND $FFFE) = $FFFE THEN GOSUB snd_alert
END
snd_auction_over: PROCEDURE
    #val = 215 : k = 6 : snd_t2 = 0 : GOSUB snd_play
END
snd_trade: PROCEDURE
    #val = 92 : k = 2 : snd_t2 = 0 : GOSUB snd_play
END
snd_event: PROCEDURE
    #val = 507 : k = 8 : snd_t2 = 0 : GOSUB snd_play
END
snd_founder: PROCEDURE
    #val = 144 : k = 10 : snd_t2 = 0 : GOSUB snd_play
END
