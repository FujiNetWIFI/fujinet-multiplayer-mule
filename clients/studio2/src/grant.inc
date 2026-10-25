; grant.inc -- the land grant (mekkogx grant.c). The cursor sweeps the map a
; plot every 12 jiffies, skipping the store and owned plots; 5 claims the
; plot under it, which the server judges as of the press. Between states the
; cursor steps on its own, by the frame each step is due, never past the
; plot the state says a computer will stop it on (grantStop). Round 1 opens
; with the transport ship leaving.

GN_CUR  EQU     PH+0                    ; the cursor's plot (255 none, 45 done)
GN_STEP EQU     PH+1
GN_SHOW EQU     PH+2
GN_DUEL EQU     PH+3                    ; the frame its next step is due
GN_DUEH EQU     PH+4
GN_LIM  EQU     PH+5                    ; the furthest plot it may step to
GN_STEPF EQU    PH+6                    ; the frame (low) of its last step
GN_DEP  EQU     PH+7                    ; the ship is leaving
GN_CLMD EQU     PH+8                    ; we have claimed (or one is out)
GN_PRESS EQU    PH+9                    ; the cursor's plot when 5 went down
GN_WAS  EQU     PH+10                   ; the plot before its last step
GN_WANT EQU     PH+11                   ; the plot we claimed and wait on
GN_PARK EQU     PH+12                   ; turned down: the server's cursor wasn't there yet
GN_K    EQU     PH+13                   ; (grant_state) the server's cursor
GN_J    EQU     PH+14
GN_N    EQU     PH+15
SLOTCUR EQU     2

        RFIT    128
grant_enter:
        SETV    MAPDOTS, 0
        FCALL   map_enter
        SETV    ANIM, AN_GRANT
        SETV    GN_SHOW, 0
        SETV    GN_CUR, 0FFH
        SETV    GN_LIM, 0FFH
        SETV    GN_CLMD, 0
        SETV    GN_DEP, 0
        SETV    GN_PARK, 0
        SETV    GN_PRESS, 0FFH
        SETV    GN_WANT, 0FFH
        SETV    GN_WAS, 0FFH
        GETV    RND                     ; round 1: the ship leaves the store first
        XRI     1
        BNZ     gre1
        FCALL   pe_now
        GHI     R9
        SMI     HI(700)
        BNF     gre0
        BNZ     gre1
        GLO     R9
        SMI     LO(700)
        BDF     gre1
gre0:   SETV    GN_DEP, 1
        LDI     T_TRANSPORT
        FCALL   title_show
        LDI     M_DEPART
        FCALL   msg_show
        SETV    FXFLAG, 0
        LDR     R9, fx_depart
        LDI     0
        FJMP    fx_start
gre1:   FJMP    gr_start

; gr_start: the grant proper (and the ship gone: mekkogx depart_done).
        RFIT    28
gr_start:
        SETV    GN_DEP, 0
        FCALL   fx_stop_all
        LDI     T_LANDGRANT
        FCALL   title_show
        LDI     M_SELECT
        FCALL   msg_show
        SETV    GN_SHOW, 1
        FJMP    alert                   ; the hand on the keypad

; gr_next: D = the free plot after plot D (45 past the last).
        RFIT    25
gr_next:
        ADI     1
        PLO     RB
        SMI     NUM_PLOTS
        BDF     gn9
        GLO     RB
        XRI     STORE_PLOT
        BZ      gr_next2
        GLO     RB
        FCALL   mp_owner
        XRI     0FFH
        BNZ     gr_next2
gn9:    GLO     RB
        FRET
gr_next2:
        GLO     RB
        BR      gr_next

; grant_state: a state (mekkogx grant_state).
        RFIT    220
grant_state:
        FCALL   map_update
        SLD     G_CURSOR                ; -1 before it starts, 45 when done
        PUTV    GN_K
        SLD     G_STEP
        BNZ     grs0
        LDI     12
grs0:   PUTV    GN_STEP
        GETV    MYSEAT
        SMI     4
        BDF     grs5
        GETV    MYSEAT                  ; mine = claimed[my seat]
        ADI     LO(G_CLAIMED)
        PLO     R8
        LDI     HI(G_CLAIMED)
        ADCI    0
        PHI     R8
        LDN     R8
        PUTV    GN_N
        SMI     NUM_PLOTS
        BDF     grs1
        SETV    GN_CLMD, 1              ; it's ours
        SETV    GN_WANT, 0FFH
grs1:   GETV    GN_WANT                 ; a claim out, not ours yet?
        SMI     NUM_PLOTS
        BDF     grs5
        GETV    GN_N
        SMI     NUM_PLOTS
        BNF     grs5
        GETV    ACTSENT                 ; only the reply to it judges it
        XRI     ACT_CLAIM
        BNZ     grs3
        SLD     H_ERR
        BZ      grs2
        FCALL   gr_klt                  ; k < want (signed): not there yet
        BZ      grs2
        SETV    GN_PARK, 1
        BR      grs5
grs2:   FJMP    gr_again
grs3:   GETV    GN_PARK                 ; parked: again once it's there
        BZ      grs5
        GETV    ACT
        BNZ     grs5
        FCALL   gr_klt
        BNZ     grs5
        SETV    GN_PARK, 0
        GETV    GN_K
        SMI     NUM_PLOTS
        BDF     grs2
        GETV    GN_WANT
        PLO     R9
        LDI     0
        PHI     R9
        LDI     ACT_CLAIM
        FCALL   act_send
grs5:   FJMP    gr_cursor

; gr_klt: D = 1 if the server's cursor (signed) is before the plot we want.
gr_klt: GETV    GN_WANT
        XRI     80H
        STR     R2
        GETV    GN_K
        XRI     80H
        SM
        BDF     gk0
        LDI     1
        FRET
gk0:    LDI     0
        FRET

; gr_again: turned down: press again.
gr_again:
        SETV    GN_WANT, 0FFH
        SETV    GN_CLMD, 0
        SETV    GN_PARK, 0
        LDI     SND_ERROR
        FCALL   snd_play
        FJMP    gr_cursor

; gr_cursor: where our cursor is, against the server's (mekkogx
; grant_state's second half). R9 = due = rx_frame + next.
        RFIT    222
gr_cursor:
        SPTR    G_NEXT
        GETV    RXFRL
        STR     R2
        LDA     R8
        ADD
        PLO     R9
        GETV    RXFRH
        STR     R2
        LDN     R8
        ADC
        PHI     R9
        GETV    GN_WANT                 ; (while our claim is out it waits)
        SMI     NUM_PLOTS
        BNF     grc8
        GETV    GN_CUR
        STR     R2
        GETV    GN_K
        XOR
        BZ      grc7                    ; with the server: due as it says
        SETV    GN_J, 0                 ; behind us, no claim since: wait for
        GETV    GN_K                    ; the server's to come and go
        PUTV    GN_N
        GETV    GN_CUR
        SMI     NUM_PLOTS
        BDF     grc5
        FCALL   gr_same
        BNZ     grc5
grc1:   GETV    GN_N
        FCALL   gr_next
        PUTV    GN_N
        GETV    GN_J
        ADI     1
        STR     RA
        SMI     8
        BDF     grc2
        GETV    GN_CUR
        STR     R2
        GETV    GN_N
        SM
        BNF     grc1
grc2:   GETV    GN_CUR
        STR     R2
        GETV    GN_N
        XOR
        BNZ     grc5
        GETV    GN_J                    ; due + j * step
        PLO     RB
grc3:   GETV    GN_STEP
        STR     R2
        GLO     R9
        ADD
        PLO     R9
        GHI     R9
        ADCI    0
        PHI     R9
        DEC     RB
        GLO     RB
        BNZ     grc3
        BR      grc7
grc5:   GETV    GN_K                    ; a claim: the cursor goes there
        PUTV    GN_CUR
        SETV    GN_WAS, 0FFH
grc7:   GLO     R9
        PUTV    GN_DUEL
        GHI     R9
        PUTV    GN_DUEH
grc8:   SLD     H_GRANTSTOP             ; the limit: the computer's stop, or
        BZ      grc9                    ; one past the server's cursor
        SMI     1
        BR      grc10
grc9:   GETV    GN_K
        FCALL   gr_next
grc10:  PUTV    GN_LIM
        GETV    GN_DEP                  ; the server's cursor has started:
        BZ      grc11                   ; the ship has gone too
        GETV    GN_K
        SMI     NUM_PLOTS
        BDF     grc12
        FJMP    gr_start
grc11:  SETV    GN_SHOW, 1
grc12:  FRET

; gr_same: D = 0 if the claims are as in the state before.
        RFIT    26
gr_same:
        SPTR    G_CLAIMED
        LDR     R9, G_CLAIMED+PRVOFF
        LDI     4
        PLO     RB
gsm1:   LDA     R8
        STR     R2
        LDA     R9
        XOR
        BNZ     gsm9
        DEC     RB
        GLO     RB
        BNZ     gsm1
gsm9:   FRET

; grant_isr: every frame (more to catch up): the cursor steps to the next
; free plot when its step is due, blanking for that frame (mekkogx grant_isr).
        RFIT    196
grant_isr:
        GETV    KEDGE                   ; 5 down: the plot it's on (or the one
        XRI     K_FIRE                  ; just left, if it stepped this frame)
        BNZ     gi1
        GETV    GN_PRESS
        XRI     0FFH
        BNZ     gi1
        GETV    GN_SHOW
        BZ      gi1
        GETV    GN_CUR
        SMI     NUM_PLOTS
        BDF     gi1
        GETV    GN_CUR
        PUTV    GN_PRESS
        GETV    GN_STEPF
        STR     R2
        GETV    FCL
        SM
        SMI     2
        BDF     gi1
        GETV    GN_WAS
        SMI     NUM_PLOTS
        BDF     gi1
        GETV    GN_WAS
        PUTV    GN_PRESS
gi1:    GETV    GN_CUR                  ; due: a step
        XRI     NUM_PLOTS
        BZ      gi4
        GETV    GN_WANT
        SMI     NUM_PLOTS
        BNF     gi4
        GETV    GN_DUEL
        STR     R2
        GETV    FCL
        SM
        GETV    GN_DUEH
        STR     R2
        GETV    FCH
        SMB
        ANI     80H
        BNZ     gi4
        GETV    GN_CUR
        FCALL   gr_next
        PLO     RC
        STR     R2
        GETV    GN_LIM
        SM
        BNF     gi4
        GETV    GN_CUR
        PUTV    GN_WAS
        GLO     RC
        PUTV    GN_CUR
        GETV    GN_STEP
        STR     R2
        GETV    GN_DUEL
        ADD
        STR     RA
        GETV    GN_DUEH
        ADCI    0
        STR     RA
        GETV    FCL
        PUTV    GN_STEPF
        FJMP    gr_hidecur
gi4:    GETV    GN_CUR
        SMI     NUM_PLOTS
        BDF     gi5
        GETV    GN_SHOW
        BNZ     gi6
gi5:    FJMP    gr_hidecur
gi6:    FJMP    gr_showcur

; gr_showcur / gr_hidecur: the cursor (an inverted plot) at GN_CUR, or off.
        RFIT    71
gr_showcur:
        GETV    GN_CUR
        SHL
        ADI     LO(plotxy)
        PLO     R8
        LDI     HI(plotxy)
        ADCI    0
        PHI     R8
        PARS
        P       SLOTCUR
        P       IM_CURSOR
        LDA     R8
        PD
        LDN     R8
        PD
        P       TM_XOR
        GOP     TOP_SPRITE
        FRET
gr_hidecur:
        PARS
        P       SLOTCUR
        P       100
        P       0
        GOP     TOP_SPRMOVE
        FRET

; grant_tick: the phase's frame (mekkogx grant_tick): 5 claims the plot the
; cursor was on when it went down, and holds it there until the server says.
        RFIT    115
grant_tick:
        FCALL   msg_tick
        GETV    GN_DEP
        BZ      grt2
        GETV    FXFLAG                  ; the ship: gone?
        XRI     1
        BZ      grt1
        GETV    FXST
        ANI     FS_RUN
        BNZ     grt9
grt1:   FJMP    gr_start
grt2:   GETV    KEDGE
        XRI     K_FIRE
        BNZ     grt9
        GETV    GN_PRESS
        PLO     RC
        SMI     NUM_PLOTS
        BNF     grt3
        GETV    GN_CUR
        PLO     RC
grt3:   SETV    GN_PRESS, 0FFH
        GETV    GN_CLMD
        BNZ     grt9
        GETV    MYSEAT
        SMI     4
        BDF     grt9
        GLO     RC
        SMI     NUM_PLOTS
        BDF     grt9
        GLO     RC                      ; claim it
        PLO     R9
        PUTV    GN_WANT
        PUTV    GN_CUR
        LDI     0
        PHI     R9
        LDI     ACT_CLAIM
        FCALL   act_send
        SETV    GN_CLMD, 1
        SETV    GN_WAS, 0FFH
        LDI     SND_BUY
        FJMP    snd_play
grt9:   FRET
