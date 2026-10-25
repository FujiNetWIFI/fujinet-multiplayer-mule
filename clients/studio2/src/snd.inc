; snd.inc -- the cues (mekkogx sound.h) on the Studio II's one tone: each is
; a rhythm of (frames on, frames off) pairs, the ISR keeping Q on while RD.0
; counts down. ALERT, the hand-on-the-keypad cue, is the longest.

; snd_play: cue D, unless muted; a soft one (STEP up) never cuts a loud one.
        RFIT    70
snd_play:
        PLO     R9
        GETV    MUTE
        BNZ     sp9
        GLO     R9
        SMI     SND_STEP
        BNF     sp1
        GETV    SNDPH                   ; soft: only if nothing loud plays
        BZ      sp1
        GETV    SNDID
        SMI     SND_STEP
        BNF     sp9
sp1:    GLO     R9
        PUTV    SNDID
        SHL
        ADI     LO(sndtab)
        PLO     R8
        LDI     HI(sndtab)
        ADCI    0
        PHI     R8
        LDA     R8
        PUTV    SNDPH
        LDN     R8
        PUTV    SNDPL
        GETV    FCL
        PUTV    SNDT
        FJMP    snd_tick
sp9:    FRET

; snd_tick: every frame: the next pair when its time comes.
        RFIT    140
snd_tick:
        GETV    SNDPH
        BZ      stk9
        GETV    SNDT                    ; FC - SNDT, as a signed byte
        STR     R2
        GETV    FCL
        SM
        ANI     80H
        BNZ     stk9
        GETV    SNDPH
        PHI     R8
        GETV    SNDPL
        PLO     R8
        LDA     R8                      ; on
        BZ      stk8
        PLO     RD                      ; the ISR's beep counter
        STR     R2
        LDA     R8                      ; + off
        ADD
        STR     R2
        GETV    FCL
        ADD
        PUTV    SNDT
        GLO     R8
        PUTV    SNDPL
        GHI     R8
        PUTV    SNDPH
stk9:   FRET
stk8:   SETV    SNDPH, 0
        FRET

; (on, off) pairs, 0 to end
sndtab: DW      s_select, s_sample, s_error, s_buy, s_alert, s_aucovr
        DW      s_trade, s_event, s_found, s_none, s_tick, s_tick
s_select: DB    3, 0, 0
s_sample: DB    2, 0, 0
s_error: DB     1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0
s_buy:  DB      4, 0, 0
s_alert: DB     8, 3, 14, 0, 0
s_aucovr: DB    6, 0, 0
s_trade: DB     2, 0, 0
s_event: DB     2, 1, 2, 1, 2, 0, 0
s_found: DB     10, 0, 0
s_tick: DB      1, 0, 0
s_none: DB      0
