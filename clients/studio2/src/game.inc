; game.inc -- what the phases share (mekkogx mule.h's macros, text.c's
; title_show).

; title_show: title D (V1 its number) centred on row 0.
title_show:
        FCALL   str_get
        LDI     0
        FJMP    text_center

; pe_now: R9 = the phase's elapsed jiffies now (rx_pe + frames since the state).
        RFIT    39
pe_now: GETV    RXFRL
        STR     R2
        GETV    FCL
        SM
        PLO     R9
        GETV    RXFRH
        STR     R2
        GETV    FCH
        SMB
        PHI     R9
        GETV    RXPEL
        STR     R2
        GLO     R9
        ADD
        PLO     R9
        GETV    RXPEH
        STR     R2
        GHI     R9
        ADC
        PHI     R9
        FRET

; alert: the hand-on-the-keypad cue, for a seated player.
        RFIT    14
alert:  GETV    MYSEAT
        SMI     4
        BDF     al9
        LDI     SND_ALERT
        FJMP    snd_play
al9:    FRET
