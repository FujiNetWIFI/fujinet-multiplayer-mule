; state.inc -- a state is in (mekkogx main.c state_arrived, clock_sync).

; st_arrive: a whole state is in SNAP: the phase runs it, then SNAP goes to
; PREV (mekkogx memcpy(&prev, &st)).
        RFIT    193
st_arrive:
        SLD     H_PHASE
        SMI     9
        BDF     sta9                    ; no such phase
        FCALL   clk_sync
        SLD     H_PHASE
        PUTV    PHASE
        SLD     H_ROUND
        PUTV    RND
        SLD     H_MYSEAT
        PUTV    MYSEAT
        SLD     H_ELAPSED
        PUTV    RXPEL
        INC     R8
        LDN     R8
        PUTV    RXPEH
        GETV    RXAGE                   ; rx_frame = now - rx_age
        STR     R2
        GETV    FCL
        SM
        PUTV    RXFRL
        GETV    FCH
        SMBI    0
        PUTV    RXFRH
        SETV    STOK, 1
        GETV    LASTPH                  ; a new phase, or a new round's grant
        STR     R2
        GETV    PHASE
        XOR
        BNZ     sta2
        GETV    PHASE
        XRI     PHASE_GRANT
        BNZ     sta3
        GETV    LASTRD
        STR     R2
        GETV    RND
        XOR
        BZ      sta3
sta2:   SETV    MSGSCR, 0
        FCALL   fx_stop_all
        SETV    ANIM, AN_NONE
        GETV    PHASE
        FCALL   ph_enter
sta3:   GETV    PHASE
        FCALL   ph_state
        FCALL   snap_prev
        GETV    PHASE
        PUTV    LASTPH
        GETV    RND
        PUTV    LASTRD
sta9:   FRET

; clk_sync: the server's clock keeps ours honest (mekkogx clock_sync): the
; gap between the two only grows when we lost frames, so whenever it is
; larger than the largest seen, the difference goes on our clock; a reply
; slower than the fastest is RXAGE older than it looks. The lobby has no
; clock. R9 = lag = server t - our frame; RC = d = lag - base.
        RFIT    141
clk_sync:
        SETV    RXAGE, 0
        GETV    TESTM
        BNZ     cks0
        SLD     H_PHASE
        BNZ     cks1
cks0:   SETV    CLKHAV, 0               ; no clock: start afresh next time
        FRET
cks1:   GETV    FCL
        STR     R2
        SLD     H_T
        SM
        PLO     R9
        GETV    FCH
        STR     R2
        INC     R8
        LDN     R8
        SMB
        PHI     R9                      ; lag
        GETV    CLKHAV
        BZ      cksb                    ; the first: it is the base
        GETV    CLKBL
        STR     R2
        GLO     R9
        SM
        PLO     RC
        GETV    CLKBH
        STR     R2
        GHI     R9
        SMB
        PHI     RC                      ; d
        BNZ     cks4                    ; d >= 256, or negative
        GLO     RC
        BZ      cks9                    ; d = 0: nothing to do
        SMI     60
        BDF     cksb                    ; 60 and up: a long stall, start again
        GLO     RC                      ; lost frames: our clock on by d
        FJMP    frame_lost              ; (the base stays: the lag is now what it was)
cks4:   ANI     80H
        BZ      cksb                    ; 256..32767: start again
        GLO     RC                      ; negative: older than the fastest by -d
        SDI     0
        PLO     RC
        GHI     RC
        SDBI    0
        BNZ     cksb                    ; more than 255: the server's clock went back
        GLO     RC
        SMI     121
        BDF     cksb
        GLO     RC
        SMI     30
        BNF     cks5
        LDI     30
        PLO     RC
cks5:   GLO     RC
        PUTV    RXAGE
cks9:   FRET
cksb:   GLO     R9
        PUTV    CLKBL
        GHI     R9
        PUTV    CLKBH
        SETV    CLKHAV, 1
        FRET
