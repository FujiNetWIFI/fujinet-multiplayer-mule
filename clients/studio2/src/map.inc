; map.inc -- the planet: 9 x 5 plots of 7 x 16 pixels (mekkogx map.c),
; the title on text row 0, the messages on rows 8-9, the time bar down the
; right edge. A plot is its terrain, then its owner's frame (a pattern per
; seat), its M.U.L.E.'s icon and (production) a dot a unit.

MAPY0   EQU     13
BARX    EQU     63
BARTOP  EQU     MAPY0
BARH    EQU     80

; map_enter: the map screen, drawn a few plots a frame from here.
        RFIT    66
map_enter:
        SETV    ANIM, AN_NONE
        FCALL   fx_stop_all
        FCALL   scr_clear
        SETV    SCRNOW, SCR_MAP
        SETV    MAPBLINK, NOBODY
        SETV    MAPBLON, 0
        SETV    BARLAST, 0FFH
        SETV    MSGROW, 8
        SETV    MAPQ, 0
        LDR     R8, MAPDIRTY
        LDI     6
        PLO     R9
me1:    LDI     0
        STR     R8
        INC     R8
        DEC     R9
        GLO     R9
        BNZ     me1
        FRET

; map_service: every frame on the map: the whole map a few plots at a time,
; then any plot marked changed (map_update).
        RFIT    105
map_service:
        GETV    SCRNOW
        XRI     SCR_MAP
        BNZ     msv9
        GETV    MAPQ
        SMI     NUM_PLOTS
        BDF     msv3
        LDI     5                       ; 5 plots this frame
        PUTV    MSVN
msv1:   GETV    MAPQ
        FCALL   map_draw_plot
        GETV    MAPQ
        ADI     1
        STR     RA
        SMI     NUM_PLOTS
        BDF     msv9
        GETV    MSVN
        SMI     1
        STR     RA
        BNZ     msv1
msv9:   FRET
msv3:   LDI     0                       ; the changed ones, 4 a frame
        PUTV    MSVP
        LDI     4
        PUTV    MSVN
msv4:   GETV    MSVP
        FCALL   map_dirty
        BZ      msv5
        GETV    MSVP
        FCALL   map_draw_plot
        GETV    MSVN
        SMI     1
        STR     RA
        BZ      msv9
msv5:   GETV    MSVP
        ADI     1
        STR     RA
        SMI     NUM_PLOTS
        BNF     msv4
        FRET

; map_dirty: D = plot p's mark (then it is cleared); 0 unmarked.
        RFIT    40
map_dirty:
        PLO     R9
        SHR
        SHR
        SHR
        ADI     LO(MAPDIRTY)
        PLO     RA
        GLO     R9
        ANI     7
        ADI     LO(bits)
        PLO     R8
        LDI     HI(bits)
        PHI     R8
        LDN     R8
        STR     R2
        LDN     RA
        AND
        BZ      mdy9
        LDN     R2                      ; clear it
        XRI     0FFH
        STR     R2
        LDN     RA
        AND
        STR     RA
        LDI     1
mdy9:   FRET
bits:   DB      1, 2, 4, 8, 10H, 20H, 40H, 80H

; map_mark: plot D to be drawn again.
        RFIT    30
map_mark:
        PLO     R9
        SHR
        SHR
        SHR
        ADI     LO(MAPDIRTY)
        PLO     RA
        GLO     R9
        ANI     7
        ADI     LO(bits2)
        PLO     R8
        LDI     HI(bits2)
        PHI     R8
        LDN     R8
        STR     R2
        LDN     RA
        OR
        STR     RA
        FRET
bits2:  DB      1, 2, 4, 8, 10H, 20H, 40H, 80H

; map_update: mark the plots whose terrain, owner, M.U.L.E. or production
; changed since the state before (mekkogx map_update).
        RFIT    62
map_update:
        LDR     R8, MP_TERRAIN
        FCALL   mu_arr
        LDR     R8, MP_OWNER
        FCALL   mu_arr
        LDR     R8, MP_MULE
        FCALL   mu_arr
        LDR     R8, MP_PROD
mu_arr: GLO     R8                      ; R9 = the same array in PREV
        PLO     R9
        GHI     R8
        ADI     HI(PRVOFF)
        PHI     R9
        LDI     0
        PLO     RC
        SEX     R9
mua1:   LDA     R8
        XOR
        INC     R9
        BZ      mua2
        SEX     R2
        GLO     RC
        FCALL   map_mark
        SEX     R9
mua2:   INC     RC
        GLO     RC
        XRI     NUM_PLOTS
        BNZ     mua1
        SEX     R2
        FRET

; map_draw_plot: plot D as the state has it (mekkogx map_draw_plot).
        RFIT    256
map_draw_plot:
        PUTV    MPP
        SHL                             ; its corner
        ADI     LO(plotxy)
        PLO     R8
        LDI     HI(plotxy)
        ADCI    0
        PHI     R8
        LDA     R8
        PUTV    MPX
        LDN     R8
        PUTV    MPY
        PARS                            ; cleared
        GETV    MPX
        PD
        GETV    MPY
        PD
        P       7
        P       16
        P       TM_CLEAR
        GOP     TOP_RECT
        GETV    MPP                     ; the terrain
        FCALL   mp_terrain
        PUTV    MPT
        ANI     3
        XRI     TERRAIN_STORE
        BNZ     mdp1
        LDI     IM_STORE
        FCALL   mp_stamp
        FRET                            ; (no one owns the store)
mdp1:   XRI     TERRAIN_STORE!TERRAIN_RIVER
        BNZ     mdp2
        LDI     IM_RIVER
        FCALL   mp_stamp
mdp2:   GETV    MPT                     ; mountains: 1-3
        SHR
        SHR
        ANI     3
        BZ      mdp3
        ADI     IM_MTN1-1
        FCALL   mp_stamp1
mdp3:   GETV    MPP                     ; owned, or blinking
        FCALL   mp_owner
        PUTV    MPO
        GETV    MAPBLON
        BZ      mdp4
        GETV    MAPBLINK
        STR     R2
        GETV    MPP
        XOR
        BNZ     mdp4
        LDI     IM_OWNB                 ; up for sale, or a computer's: outlined
        FCALL   mp_stamp
        GETV    MPO
        SMI     4
        BDF     mdp9                    ; nobody's: no icon
        BR      mdp5
mdp4:   GETV    MPO
        SMI     4
        BDF     mdp9
        GETV    MPO
        ADI     IM_OWN0
        FCALL   mp_stamp
mdp5:   GETV    MPP                     ; the icon: a house, or the M.U.L.E.
        FCALL   mp_mule
        PUTV    MPG
        SMI     4
        BNF     mdp6
        LDI     IM_ICHOUSE
        BR      mdp7
mdp6:   GETV    MPG
        ADI     IM_ICFOOD
mdp7:   FCALL   mp_stamp2
        GETV    MAPDOTS                 ; production: a dot a unit
        BZ      mdp9
        GETV    MPG
        SMI     4
        BDF     mdp9
        GETV    MPP
        FCALL   mp_prod
        ANI     80H
        BNZ     mdp9                    ; lost
        GETV    MPP
        FCALL   mp_prod
        ANI     0FH
        BZ      mdp9
        SMI     9
        BNF     mdp8
        LDI     8-9
mdp8:   ADI     9+IM_DOTS1-1
        FCALL   mp_stamp3
mdp9:   FRET

; mp_stamp / 1 / 2 / 3: picture D, ORed at the plot's corner / 3 lines
; down (mountains) / its icon place / its dots' place.
        RFIT    68
mp_stamp:
        PLO     R9
        LDI     0
        BR      mps
mp_stamp1:
        PLO     R9
        LDI     2
        BR      mps
mp_stamp2:
        PLO     R9
        LDI     10
        BR      mps1
mp_stamp3:
        PLO     R9
        LDI     11
mps:    STR     R2
        PARS
        GLO     R9
        PD
        GETV    MPX
        BR      mps2
mps1:   STR     R2
        PARS
        GLO     R9
        PD
        GETV    MPX
        ADI     1
mps2:   PD
        GETV    MPY
        ADD
        PD
        P       TM_OR
        GOP     TOP_STAMP
        FRET

; mp_terrain / mp_owner / mp_mule / mp_prod: plot D's, from the state.
        RFIT    111
mp_terrain:
        ADI     LO(MP_TERRAIN)
        PLO     R8
        LDI     HI(MP_TERRAIN)
        BR      mpf
mp_owner:
        ADI     LO(MP_OWNER)
        PLO     R8
        LDI     HI(MP_OWNER)
        BR      mpf
mp_mule:
        ADI     LO(MP_MULE)
        PLO     R8
        LDI     HI(MP_MULE)
        BR      mpf
mp_prod:
        ADI     LO(MP_PROD)
        PLO     R8
        LDI     HI(MP_PROD)
mpf:    ADCI    0
        PHI     R8
        LDN     R8
        FRET

; plot p's corner: x, y
plotxy: DB      0,MAPY0,7,MAPY0,14,MAPY0,21,MAPY0,28,MAPY0,35,MAPY0,42,MAPY0,49,MAPY0,56,MAPY0
        DB      0,MAPY0+16,7,MAPY0+16,14,MAPY0+16,21,MAPY0+16,28,MAPY0+16,35,MAPY0+16,42,MAPY0+16,49,MAPY0+16,56,MAPY0+16
        DB      0,MAPY0+32,7,MAPY0+32,14,MAPY0+32,21,MAPY0+32,28,MAPY0+32,35,MAPY0+32,42,MAPY0+32,49,MAPY0+32,56,MAPY0+32
        DB      0,MAPY0+48,7,MAPY0+48,14,MAPY0+48,21,MAPY0+48,28,MAPY0+48,35,MAPY0+48,42,MAPY0+48,49,MAPY0+48,56,MAPY0+48
        DB      0,MAPY0+64,7,MAPY0+64,14,MAPY0+64,21,MAPY0+64,28,MAPY0+64,35,MAPY0+64,42,MAPY0+64,49,MAPY0+64,56,MAPY0+64

; ---- the time bar: one line for each PTU of turn time left (mekkogx bar_draw) ----
; bar_draw: D lines of the bar lit, from the bottom.
        RFIT    90
bar_draw:
        PLO     R9
        SMI     BARH+1
        BNF     bd1
        LDI     BARH
        PLO     R9
bd1:    GETV    BARLAST
        STR     R2
        GLO     R9
        XOR
        BZ      bd9
        GLO     R9
        STR     RA                      ; BARLAST
        PARS                            ; the dark part
        P       BARX
        P       BARTOP
        P       1
        GLO     R9
        SDI     BARH
        PD
        P       TM_CLEAR
        GOP     TOP_RECT
        GLO     R9                      ; the lit part
        BZ      bd9
        PARS
        P       BARX
        GLO     R9
        SDI     BARTOP+BARH
        PD
        P       1
        GLO     R9
        PD
        P       TM_SET
        GOP     TOP_RECT
bd9:    FRET

; bar_tick: the original's tick (its VBI, $5E98): with D units of the bar
; left (PTU on a turn, ticks in an auction) and D under 30, a tick every D
; frames, high and low by turns (mekkogx bar_tick). Every frame the bar runs.
        RFIT    89
bar_tick:
        PLO     R9
        GETV    BARFR                   ; d = frames gone by
        STR     R2
        GETV    FCL
        SM
        BZ      bt9
        PLO     RB
        GETV    FCL
        PUTV    BARFR
        GLO     R9
        BZ      bt1
        SMI     30
        BNF     bt2
bt1:    SETV    BARTK, 1                ; the first tick comes at 29, high
        SETV    BARTG, 0
bt9:    FRET
bt2:    GLO     RB                      ; tk > d: tk -= d
        STR     R2
        GETV    BARTK
        SM
        BZ      bt3
        BNF     bt3
        STR     RA
        FRET
bt3:    GLO     R9
        PUTV    BARTK
        GETV    BARTG
        ADI     SND_TICK
        FCALL   snd_play
        GETV    BARTG
        XRI     1
        STR     RA
        FRET
