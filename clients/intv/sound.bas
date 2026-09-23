' ===========================================================================
' sound.bas -- short effects on PSG channel A.
'
' An effect sets a tone and its length; the frame handler (snd_isr) counts it
' down and plays an optional second tone, so nothing here blocks the game.
' Tone values are 3579545 / 32 / Hz.
' ===========================================================================

    DIM snd_t, snd_t2, #snd_f2

' Frame handler side.
snd_isr: PROCEDURE
    IF snd_t = 0 THEN RETURN
    snd_t = snd_t - 1
    IF snd_t = 0 THEN
        IF snd_t2 THEN
            SOUND 0, #snd_f2, 12
            snd_t = snd_t2 : snd_t2 = 0
        ELSE
            SOUND 0, 1, 0
        END IF
    END IF
END

' snd_play: #val = tone, k = frames; #snd_f2 / snd_t2 = optional second tone.
snd_play: PROCEDURE
    snd_t = 0
    SOUND 0, #val, 12
    snd_t = k
END

snd_select: PROCEDURE        ' a blip (~880 Hz)
    #val = 127 : k = 4 : snd_t2 = 0 : GOSUB snd_play
END
snd_sample: PROCEDURE        ' a low dig
    #val = 400 : k = 6 : snd_t2 = 0 : GOSUB snd_play
END
snd_error: PROCEDURE
    #val = 900 : k = 12 : snd_t2 = 0 : GOSUB snd_play
END
snd_buy: PROCEDURE          ' ka-ching
    #val = 100 : k = 5 : #snd_f2 = 75 : snd_t2 = 8 : GOSUB snd_play
END
snd_myturn: PROCEDURE
    #val = 170 : k = 10 : #snd_f2 = 127 : snd_t2 = 14 : GOSUB snd_play
END
snd_declare: PROCEDURE
    #val = 213 : k = 8 : #snd_f2 = 160 : snd_t2 = 8 : GOSUB snd_play
END
snd_auction_over: PROCEDURE
    #val = 160 : k = 10 : #snd_f2 = 213 : snd_t2 = 16 : GOSUB snd_play
END
snd_trade: PROCEDURE
    #val = 90 : k = 3 : snd_t2 = 0 : GOSUB snd_play
END
snd_tick: PROCEDURE         ' the land grant cursor stepping
    #val = 250 : k = 2 : snd_t2 = 0 : GOSUB snd_play
END
snd_event: PROCEDURE
    #val = 500 : k = 20 : #snd_f2 = 700 : snd_t2 = 20 : GOSUB snd_play
END
snd_founder: PROCEDURE
    #val = 213 : k = 20 : #snd_f2 = 106 : snd_t2 = 40 : GOSUB snd_play
END
snd_boot: PROCEDURE
    #val = 320 : k = 10 : #snd_f2 = 213 : snd_t2 = 20 : GOSUB snd_play
END
