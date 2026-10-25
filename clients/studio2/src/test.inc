; test.inc -- bank 1: the offline test mode (mekkogx teststart.c, test.c)
; and autoplay. When the harness pokes $5A into TESTHK during the title, the
; game skips FujiNet and fakes a state: round 1 on a generated map (build/
; teststate.inc). TESTHK+1 is the phase + 1 (0: develop), +2 the colony
; event / auction stage / seat whose turn it is, +3 the player event.
; Actions are answered here, so the screens and animations can be checked
; in seconds.

        FJMP    test_start              ; E_TSTART
        FJMP    test_poll               ; E_TPOLL
        FJMP    autoplay                ; E_AUTO

; tp8: SNAP byte at R9 = D. tp16: the word at R9 = RB (LE).
tp8:    FJMP    cr_poke
        RFIT    11
tp16:   FCALL   cr_at
        GLO     RB
        FCALL   cr_put
        GHI     RB
        FJMP    cr_put

        RFIT    115
test_start:
        LDR     R8, TSTST               ; the state, copied from this window
        LDR     R9, SNAP
        LDR     RB, NREPLEN
        FCALL   ccopy_wait
        GETV    TESTHK+1                ; the phase: 0 develop, else n - 1
        BNZ     ts1
        LDI     PHASE_DEVELOP+1
ts1:    SMI     1
        PUTV    T4                      ; T4 = the phase
        PLO     RC
        LDR     R9, H_PHASE
        GLO     RC
        FCALL   tp8
        GETV    T4                      ; the lobby and summary: species + arg
        XRI     PHASE_LOBBY
        BZ      ts2
        XRI     PHASE_LOBBY!PHASE_SUMMARY
        BNZ     ts4
ts2:    LDI     0
        PUTV    T5
ts3:    GETV    T5                      ; species of seat k = 1 + 2k + arg
        SHL
        ADI     1
        STR     R2
        GETV    TESTHK+2
        ADD
        ANI     7
        PLO     RC
        LDR     R9, S_PLAYERS+PL_SPECIES
        GETV    T5
        FCALL   tseat
        GLO     RC
        FCALL   tp8
        GETV    T5
        ADI     1
        STR     RA
        XRI     4
        BNZ     ts3
ts4:    FJMP    ts4x

        RFIT    174
ts4x:   GETV    TESTHK+2                ; whose turn, the colony event
        ANI     3
        PLO     RC
        LDR     R9, TN_SEAT
        GLO     RC
        FCALL   tp8
        LDR     R9, C_EVENT
        GETV    TESTHK+2
        FCALL   tp8
        LDR     R9, TN_EVENT
        GETV    TESTHK+3                ; the player event
        FCALL   tp8
        GETV    T4                      ; a land auction: plot 13 from $240
        XRI     PHASE_LANDAUCTION
        BNZ     ts5
        LDR     R9, A_KIND
        LDI     1
        FCALL   tp8
        LDR     R9, A_STEP
        LDI     4
        FCALL   tp8
        LDR     R9, A_PLOT
        LDI     13
        FCALL   tp8
        LDR     RB, 240
        LDR     R9, A_START
        FCALL   tp16
ts5:    GETV    T4                      ; the stage: the argument, in auctions
        XRI     PHASE_DECLARE
        BZ      ts6
        XRI     PHASE_DECLARE!PHASE_TRADING
        BZ      ts6
        XRI     PHASE_TRADING!PHASE_LANDAUCTION
        BNZ     ts7
ts6:    LDR     R9, A_STAGE
        GETV    TESTHK+2
        FCALL   tp8
ts7:    LDR     R8, t_name              ; who we are, where
        LDR     R9, CR_NAME
        FCALL   cr_puts
        LDR     R8, t_table
        LDR     R9, CR_TABLE
        FCALL   cr_puts
        SETV    TESTHK, 0A5H            ; tell the harness
        FRET
t_name: DB      "PL0", 0
t_table: DB     "test", 0

; tseat: R9 += D * PL_SIZE (seat D's field). Keeps RC.
        RFIT    16
tseat:  PLO     RB
ts9:    GLO     RB
        BZ      ts10
        DEC     RB
        GLO     R9
        ADI     PL_SIZE
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
        BR      ts9
ts10:   FRET

; test_poll: answer the action as the server would, roughly, and age the
; clocks (mekkogx test.c test_poll). T2:T3 = w, the frames since the last.
        RFIT    249
test_poll:
        GETV    RXFRL                   ; w = since_rx()
        STR     R2
        GETV    FCL
        SM
        PUTV    T2
        GETV    RXFRH
        STR     R2
        GETV    FCH
        SMB
        PUTV    T3
        SETV    NHAVE, NH_STATE
        GETV    ACT
        PUTV    ACTSENT
        LDR     R9, H_ERR
        LDI     0
        FCALL   tp8
        GETV    ACT
        XRI     ACT_BUYMU
        BNZ     tq1
        LDI     TF_STARTED|TF_HASMULE
        BR      tqf
tq1:    XRI     ACT_BUYMU!ACT_RETURN
        BNZ     tq2
        LDI     TF_STARTED
tqf:    PLO     RC
        LDR     R9, TN_FLAGS
        GLO     RC
        FCALL   tp8
        BR      tq9
tq2:    XRI     ACT_RETURN!ACT_OUTFIT
        BNZ     tq3
        GETV    ACTARGL
        PLO     RC
        LDR     R9, TN_CARRYING
        GLO     RC
        FCALL   tp8
        BR      tq9
tq3:    XRI     ACT_OUTFIT!ACT_INST
        BNZ     tq4
        LDR     R9, TN_FLAGS
        LDI     TF_STARTED
        FCALL   tp8
        LDR     R9, MP_OWNER        ; the plot is ours, with our M.U.L.E.
        LDI     0
        FCALL   tq_plot
        SLD     TN_CARRYING
        PLO     RC
        LDR     R9, MP_MULE
        GLO     RC
        FCALL   tq_plot
        BR      tq9
tq4:    XRI     ACT_INST!ACT_ASSAY
        BNZ     tq5
        LDR     RB, 3
        BR      tqr
tq5:    XRI     ACT_ASSAY!ACT_PUB
        BNZ     tq6
        LDR     R9, TN_FLAGS
        LDI     TF_STARTED|TF_ENDED
        FCALL   tp8
        LDR     RB, 75
tqr:    LDR     R9, H_RESULT
        FCALL   tp16
        BR      tq9
tq6:    XRI     ACT_PUB!ACT_CLAIM
        BNZ     tq9
        LDR     R9, G_CLAIMED
        GETV    ACTARGL                 ; claimed: the plot is ours
        FCALL   tp8
        LDR     R9, MP_OWNER
        LDI     0
        FCALL   tq_plot
tq9:    FJMP    tq_clocks

; tq_plot: the map array at R9, entry ACTARG, = D.
tq_plot:
        PLO     RC
        GETV    ACTARGL
        STR     R2
        GLO     R9
        ADD
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
        GLO     RC
        FJMP    tp8

; tq_clocks: the turn's time left, the phase's elapsed, the server's t, the
; grant's cursor.
        RFIT    124
tq_clocks:
        SPTR    TN_LEFT                 ; left: less w while developing
        LDA     R8
        PLO     RB
        LDN     R8
        PHI     RB
        SLD     H_PHASE
        XRI     PHASE_DEVELOP
        BNZ     tk2
        GETV    T2                      ; left - w, if it goes
        STR     R2
        GLO     RB
        SM
        PLO     RB
        GETV    T3
        STR     R2
        GHI     RB
        SMB
        PHI     RB
        BDF     tk3
tk2:    LDI     0
        PLO     RB
        PHI     RB
tk3:    LDR     R9, TN_LEFT
        FCALL   tp16
        SLD     H_PHASE                 ; elapsed: on, unless a turn ran out
        XRI     PHASE_DEVELOP
        BNZ     tk4
        SLD     TN_LEFT
        INC     R8
        SEX     R8
        OR
        SEX     R2
        BZ      tk5
tk4:    LDR     R9, H_ELAPSED
        FCALL   tq_addw
tk5:    LDR     R9, H_T
        FCALL   tq_addw
        LDR     R9, TN_ELAPSED
        FCALL   tq_addw
        SLD     H_PHASE
        XRI     PHASE_GRANT
        BNZ     tk9
        FJMP    tq_grant
tk9:    SETV    ACT, ACT_NONE
        FRET

; tq_addw: the word at R9 += w.
        RFIT    23
tq_addw:
        GLO     R9
        PLO     R8
        GHI     R9
        PHI     R8
        GETV    T2
        STR     R2
        LDA     R8
        ADD
        PLO     RB
        GETV    T3
        STR     R2
        LDN     R8
        ADC
        PHI     RB
        FJMP    tp16

; tq_grant: as the server's cursor: it waits (-1) for the ship, then steps
; every 12 (next: jiffies to its next step). T2:T3 = v.
        RFIT    256
tq_grant:
        SLD     G_CURSOR
        ANI     80H
        BZ      tg3
        SPTR    G_NEXT                  ; next > v: next -= v, v = 0
        LDA     R8
        PLO     RB
        LDN     R8
        PHI     RB
        GETV    T2
        STR     R2
        GLO     RB
        SM
        PLO     RC
        GETV    T3
        STR     R2
        GHI     RB
        SMB
        PHI     RC                      ; RC = next - v
        BNF     tg2
        GLO     RC
        BNZ     tg1
        GHI     RC
        BZ      tg2
tg1:    GLO     RC
        PLO     RB
        GHI     RC
        PHI     RB
        LDR     R9, G_NEXT
        FCALL   tp16
        BR      tg9
tg2:    GLO     RC                      ; v -= next (= -RC); the cursor starts
        SDI     0
        PUTV    T2
        GHI     RC
        SDBI    0
        PUTV    T3
        LDR     R9, G_CURSOR
        LDI     0
        FCALL   tp8
        LDR     RB, 12
        LDR     R9, G_NEXT
        FCALL   tp16
tg3:    SLD     G_CURSOR                ; v += cursor * 12 + 12 - next
        PLO     RC
        LDI     0
        PHI     RC
        LDI     12
        PLO     RB
tg4:    GLO     RB
        BZ      tg5
        DEC     RB
        GETV    T2
        STR     R2
        GLO     RC
        ADD
        PUTV    T2
        GETV    T3
        ADCI    0
        PUTV    T3
        BR      tg4
tg5:    SLD     G_NEXT
        SDI     12
        STR     R2
        GETV    T2
        ADD
        PUTV    T2
        GETV    T3
        ADCI    0
        PUTV    T3
        LDI     0                       ; cursor = v / 12, at most 45
        PLO     RC
tg6:    GETV    T2
        SMI     12
        PLO     RB
        GETV    T3
        SMBI    0
        BNF     tg7
        PUTV    T3
        GLO     RB
        PUTV    T2
        INC     RC
        GLO     RC
        XRI     45
        BNZ     tg6
tg7:    LDR     R9, G_CURSOR
        GLO     RC
        FCALL   tp8
        GETV    T2                      ; next = 12 - v % 12
        SDI     12
        PLO     RB
        LDI     0
        PHI     RB
        LDR     R9, G_NEXT
        FCALL   tp16
tg9:    SETV    ACT, ACT_NONE
        FRET

autoplay:
        FRET

TSTST_SEGEND:                           ; (data from here: no page rules)
        INCLUDE "teststate.inc"
