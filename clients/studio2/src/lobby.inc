; lobby.inc -- the lobby (mekkogx status.c): the seats, their species, ready.
; 4 and 6 change our species, 5 toggles ready, 0 is the menu. A band of 18
; lines a seat: its frame (the pattern its plots wear on the map) with the
; species standing in it, the name, the species, READY or AI or OPEN.

LB_LAST EQU     PH+0                    ; 4: what each band shows (species + flags * 8)
LB_SPEC EQU     PH+4                    ; our species
LB_SENT EQU     PH+5
LB_CDL  EQU     PH+6                    ; the countdown shown (FFFE: none yet,
LB_CDH  EQU     PH+7                    ;  FFFF: waiting)
LB_SPC  EQU     PH+8                    ; watching: the waiting line says so
LB_K    EQU     PH+9
LB_Y    EQU     PH+10
LB_F    EQU     PH+11

        RFIT    94
lobby_enter:
        SETV    ANIM, AN_NONE
        FCALL   fx_stop_all
        FCALL   scr_clear
        SETV    SCRNOW, SCR_TEXT
        SETV    MSGROW, 8
        LDR     R8, LB_LAST
        LDI     4
        PLO     R9
lbe1:   LDI     0FFH
        STR     R8
        INC     R8
        DEC     R9
        GLO     R9
        BNZ     lbe1
        LDI     W_MULE
        FCALL   title_show
        SETV    LB_SENT, 0
        SETV    LB_CDL, 0FEH
        SETV    LB_CDH, 0FFH
        GETV    MYSEAT
        SMI     4
        BDF     lbe9
        LDR     R9, S_PLAYERS+PL_SPECIES
        GETV    MYSEAT
        FCALL   pl_byte
        ANI     7
        PUTV    LB_SPEC
lbe9:   FRET

; pl_byte: D = seat D's byte at R9 (a field of seat 0). R8 left on it.
        RFIT    21
pl_byte:
        PLO     RB
        GLO     R9
        PLO     R8
        GHI     R9
        PHI     R8
plb1:   GLO     RB
        BZ      plb2
        DEC     RB
        GLO     R8
        ADI     PL_SIZE
        PLO     R8
        GHI     R8
        ADCI    0
        PHI     R8
        BR      plb1
plb2:   LDN     R8
        FRET

; lobby_state: a band again where its seat changed (mekkogx lobby_state).
        RFIT    115
lobby_state:
        SETV    LB_K, 0
lbs1:   LDR     R9, S_PLAYERS+PL_FLAGS
        GETV    LB_K                    ; j + c * 8
        FCALL   pl_byte
        SHL
        SHL
        SHL
        PUTV    LB_F
        LDR     R9, S_PLAYERS+PL_SPECIES
        GETV    LB_K
        FCALL   pl_byte
        ANI     7
        STR     R2
        GETV    LB_F
        ADD
        PUTV    LB_F
        GETV    LB_K
        ADI     LO(LB_LAST)
        PLO     RA
        LDN     RA
        STR     R2
        GETV    LB_F
        XOR
        BNZ     lbs2
        LDR     R9, S_PLAYERS+PL_NAME
        GETV    LB_K                    ; the same, and seated: as it is
        FCALL   pl_byte
        BNZ     lbs3
lbs2:   GETV    LB_F
        PLO     RC
        GETV    LB_K
        ADI     LO(LB_LAST)
        PLO     RA
        GLO     RC
        STR     RA
        FCALL   lb_band
lbs3:   GETV    LB_K
        ADI     1
        STR     RA
        XRI     4
        BNZ     lbs1
        FRET

; lb_band: seat LB_K's band.
        RFIT    242
lb_band:
        GETV    LB_K                    ; y = 12 + 18k
        SHL
        SHL
        SHL
        STR     R2
        GETV    LB_K
        ADD
        SHL
        ADI     12
        PUTV    LB_Y
        PARS                            ; the band cleared
        P       0
        GETV    LB_Y
        PD
        P       64
        P       18
        P       TM_CLEAR
        GOP     TOP_RECT
        PARS                            ; the seat's frame
        GETV    LB_K
        ADI     IM_OWN0
        PD
        P       1
        GETV    LB_Y
        ADI     1
        PD
        P       TM_OR
        GOP     TOP_STAMP
        LDR     R9, S_PLAYERS+PL_NAME
        GETV    LB_K
        FCALL   pl_byte
        BNZ     lbb1
        PARS                            ; open: no one
        GETV    LB_K
        PD
        P       100
        P       0
        GOP     TOP_SPRMOVE
        LDI     B_OPEN
        FCALL   str_get
        LDI     10
        PLO     RC
        FJMP    lb_txt
lbb1:   PARS                            ; the species, standing in it
        GETV    LB_K
        PD
        GETV    LB_F
        ANI     7
        ADI     IM_STAND_MECHTRON
        PD
        P       2
        GETV    LB_Y
        ADI     2
        PD
        P       TM_OR
        GOP     TOP_SPRITE
        GETV    LB_K                    ; ours: the name inverse
        STR     R2
        GETV    MYSEAT
        XOR
        BNZ     lbb2
        PARS
        P       9
        GETV    LB_Y
        PD
        P       34
        P       12
        P       TM_SET
        GOP     TOP_RECT
lbb2:   FJMP    lb_band2

        RFIT    120
lb_band2:
        PARS                            ; the name (8 at most)
        P       10
        GETV    LB_Y
        PD
        P       TM_XOR
        LDR     R9, S_PLAYERS+PL_NAME
        GETV    LB_K
        FCALL   pl_byte                 ; (R8 on its name)
        LDI     TX_PARAM
        PHI     RF
        LDI     8
        PLO     RC
lbb3:   LDA     R8
        BZ      lbb4
        PD
        DEC     RC
        GLO     RC
        BNZ     lbb3
lbb4:   GOP     TOP_PTEXT
        GETV    LB_F                    ; the species, small
        ANI     7
        ADI     SP_0
        FCALL   str_get
        PARS
        P       10
        GETV    LB_Y
        ADI     12
        PD
        P       TM_OR|TM_SHORT
        FCALL   lb_ptxt
        GETV    LB_F                    ; AI, READY
        ANI     PF_AI*8
        BZ      lbb5
        LDI     B_AI
        BR      lbb6
lbb5:   GETV    LB_F
        ANI     PF_READY*8
        BZ      lbb9
        LDI     B_READYK
lbb6:   FCALL   str_get
        LDI     44
        PLO     RC
        FJMP    lb_txt
lbb9:   FRET

; lb_txt: TXT in the band at x RC (normal size). lb_ptxt: TXT as the rest of
; an open PTEXT, then its op.
        RFIT    43
lb_txt: PARS
        GLO     RC
        PD
        GETV    LB_Y
        PD
        P       TM_OR
lb_ptxt:
        LDR     R8, TXT
        LDI     TX_PARAM
        PHI     RF
lt1:    LDA     R8
        BZ      lt2
        PD
        BR      lt1
lt2:    GOP     TOP_PTEXT
        FRET

; lobby_tick: the countdown, our species, ready (mekkogx lobby_tick).
        RFIT    56
lobby_tick:
        FCALL   msg_tick
        GETV    MYSEAT                  ; watching or not: the waiting line again
        SMI     4
        BDF     lbt0
        LDI     0
        BR      lbt0a
lbt0:   LDI     1
lbt0a:  STR     R2
        GETV    LB_SPC
        XOR
        BZ      lbt1
        LDN     R2
        STR     RA
        SETV    LB_CDL, 0FEH
        SETV    LB_CDH, 0FFH
lbt1:   SPTR    H_COUNTDOWN+1           ; a countdown, or waiting
        LDN     R8
        ANI     80H
        BZ      lbt2
        FJMP    lb_wait
lbt2:   FJMP    lb_count

; lb_wait: no countdown: WAITING (or WATCHING) once.
        RFIT    46
lb_wait:
        GETV    LB_CDL
        XRI     0FFH
        BNZ     lbw1
        GETV    LB_CDH
        XRI     0FFH
        BZ      lbw9
lbw1:   SETV    LB_CDL, 0FFH
        SETV    LB_CDH, 0FFH
        GETV    LB_SPC
        BZ      lbw2
        LDI     M_SPECTATE
        BR      lbw3
lbw2:   LDI     M_WAITING
lbw3:   FCALL   msg_show
lbw9:   FJMP    lb_keys

; lb_count: the seconds to the start (redrawn only when they change).
        RFIT    133
lb_count:
        SPTR    H_COUNTDOWN             ; w - frames since the state
        LDA     R8
        PLO     R9
        LDN     R8
        PHI     R9
        GETV    RXFRL
        STR     R2
        GETV    FCL
        SM
        PLO     RB
        GETV    RXFRH
        STR     R2
        GETV    FCH
        SMB
        PHI     RB
        GLO     RB
        STR     R2
        GLO     R9
        SM
        PLO     R9
        GHI     RB
        STR     R2
        GHI     R9
        SMB
        PHI     R9
        BDF     lbc1
        LDI     0
        PLO     R9
        PHI     R9
lbc1:   LDI     0                       ; / 60
        PLO     RC
lbc2:   GLO     R9
        SMI     60
        PLO     RB
        GHI     R9
        SMBI    0
        BNF     lbc3
        PHI     R9
        GLO     RB
        PLO     R9
        INC     RC
        BR      lbc2
lbc3:   GETV    LB_CDH                  ; changed?
        BNZ     lbc4
        GETV    LB_CDL
        STR     R2
        GLO     RC
        XOR
        BZ      lbc9
lbc4:   GETV    LB_CDH                  ; the count has begun: the alert
        XRI     0FFH
        BNZ     lbc5
        GETV    LB_CDL
        SMI     0FEH
        BNF     lbc5
        FCALL   alert
lbc5:   GLO     RC
        PUTV    LB_CDL
        PUTV    V1L
        SETV    LB_CDH, 0
        SETV    V1H, 0
        LDI     M_LEAVEHELP
        FCALL   msg_show
lbc9:   FJMP    lb_keys

; lb_keys: 4 and 6 our species (one change a press), 5 ready or not.
        RFIT    96
lb_keys:
        GETV    MYSEAT
        SMI     4
        BDF     lbk9
        GETV    KEDGE
        XRI     4
        BZ      lbk1
        XRI     4!6
        BNZ     lbk3
        GETV    LB_SPEC                 ; 6: the next
        ADI     1
        BR      lbk2
lbk1:   GETV    LB_SPEC                 ; 4: the one before
        ADI     7
lbk2:   ANI     7
        PUTV    LB_SPEC
        PLO     R9
        LDI     0
        PHI     R9
        LDI     ACT_SPEC
        FCALL   act_send
        LDI     SND_SELECT
        FJMP    snd_play
lbk3:   GETV    KEDGE
        XRI     K_FIRE
        BNZ     lbk9
        LDR     R9, S_PLAYERS+PL_FLAGS
        GETV    MYSEAT                  ; ready, or not
        FCALL   pl_byte
        ANI     PF_READY
        BZ      lbk4
        LDI     ACT_UNRDY
        BR      lbk5
lbk4:   LDI     ACT_READY
lbk5:   FCALL   act_send
        LDI     SND_SELECT
        FJMP    snd_play
lbk9:   FRET
