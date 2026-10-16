; ---------------------------------------------------------------------------
; p2grant.inc -- page 2: the land grant (mekkogx grant.c) and the production
; phase with its colony events (events.c).
;
; The cursor sweeps the map a plot every step (12 jiffies), skipping the
; store and owned plots; the button claims the plot under it. The server
; judges the claim as of the press, and takes a plot it gave a computer
; within that time. Between polls the cursor steps on its own, never past
; the plot the state says a computer will stop it on (an older server
; doesn't say: one plot past its cursor). Round 1 opens with the transport
; ship leaving.
; ---------------------------------------------------------------------------

G_CUR   EQU PV+0                ; the cursor's plot (255 before it starts)
G_NEXT  EQU PV+1                ; frames to its next step
G_STEP  EQU PV+2                ; frames a step
G_SHOW  EQU PV+3
GR_DEP  EQU PV+4                ; the ship is leaving first
GR_CLD  EQU PV+5                ; we have claimed
GR_SRV  EQU PV+6                ; the cursor in the last state
GR_PRS  EQU PV+7                ; the cursor's plot when the button went down
GR_WAS  EQU PV+8                ; the plot before the cursor's last step
GR_WNT  EQU PV+9                ; the plot we claimed and wait on
GR_LIM  EQU PV+10               ; the furthest plot the cursor may step to
GR_PRK  EQU PV+11               ; our claim came too early: sent again when
                                ; the server's cursor gets there
PR_EV   EQU PV+16               ; the colony event (0 none)
PR_PL   EQU PV+17               ; its plot
PR_STR  EQU PV+18               ; its set piece has started
PR_DON  EQU PV+19               ; over: the production dots
PR_LAST EQU PV+20               ; 2: the fire / quake's last flicker
PR_K    EQU PV+22

SLOT_CUR EQU 2
PICN_FIRE EQU 2                 ; PICTAB: the fire's two pictures

; ---------------------------------------------------------------------------
; GRANTE -- grant_enter.
; ---------------------------------------------------------------------------
GRANTE: ENTER
        CLR
        STB MAPDOTS
        PI MAPENTER
        LIS AN_GRANT
        STB ANIM
        CLR
        STB G_SHOW
        STB G_NEXT
        STB GR_CLD
        STB GR_DEP
        STB GR_PRK
        LI 0FFH
        STB G_CUR
        STB GR_PRS
        STB GR_WNT
        STB GR_WAS
        STB GR_LIM
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        LDB RND                 ; round 1, early: the ship leaves first
        CI 1
        BNZ GE2
        PI PENOW
        LI 700&0FFH
        LR 2,A
        LI 700>>8
        LR 3,A
        PI GE16
        BC GE2
        LIS 1
        STB GR_DEP
        LI TI_TRANSPORT
        LR 0,A
        CLR
        LR 1,A
        PI TITLE
        LI MS_DEPART
        LR 0,A
        PI GMSG
        CLR                     ; (the summary's landing left it 1)
        STB FXFLAG
        CLR
        LR 0,A
        DCI FXDEPART
        LR H,DC
        PI FXSTART
        LEAVE
GE2:    PI GRTITLE
        LEAVE

; GRTITLE -- "LAND GRANT", the prompt, and the alert for a player.
GRTITLE: ENTER
        LI TI_LANDGRANT
        LR 0,A
        CLR
        LR 1,A
        PI TITLE
        LI MS_SELECT
        LR 0,A
        PI GMSG
        LDB MYSEAT
        CI 3
        BNC GRT1
        LIS SND_ALERT
        LR 0,A
        PI SNDPLAY
GRT1:   LEAVE

; GMSG -- message r0, without numbers.
GMSG:   ENTER
        CLR
        STB EXPV1
        ST
        STB EXPV2
        ST
        PI MSGSHOW
        LEAVE

; GRDDONE -- the ship has gone: the grant proper begins (depart_done).
GRDDONE: ENTER
        CLR
        STB GR_DEP
        PI FXSTOP
        PI GRTITLE
        LIS 1
        STB G_SHOW
        LEAVE

; NEXTFR -- A = the free plot after plot r5 (NPLOTS past the last).  Leaf.
;   Clobbers r5, DC0.
NEXTFR:
NF1:    LR A,5
        INC
        LR 5,A
        CI NPLOTS-1
        BNC NFX
        CI STOREPL
        BZ NF1
        DCI ST+M_OWNER
        ADC
        LM
        INC                     ; 255: no one's
        BNZ NF1
NFX:    LR A,5
        POP

; ---------------------------------------------------------------------------
; GRANTS -- grant_state: the server's cursor and our claim.
; ---------------------------------------------------------------------------
GRANTS: ENTER
        PI MAPUPD
        LDB ST+GR_CUR           ; k: -1 (255) before it starts, 45 when done
        STB GR_SRV
        LDB ST+GR_STEP
        NI 0FFH
        BNZ GS1
        LI 12
GS1:    STB G_STEP
        PI GRCLAIM              ; our claim's answer first
        LDW 0, 1, ST+GR_NEXT    ; w: the countdown ...
        LR A,1
        NI 0FFH
        BZ GS2
        CLR                     ; (far off, the ship leaving: wait for a
        LR 0,A                  ; nearer state)
        BR GS4
GS2:    LDB RXAGE               ; ... less the state's age: the step it
        LR 2,A                  ; waited for may have come since
        COM
        AS 0                    ; w + ~age carries: w > age
        BC GS3
        LIS 1
        LR 0,A                  ; w = 1
        BR GS4
GS3:    LR A,2
        COM
        INC
        AS 0
        LR 0,A                  ; w -= age
GS4:    LDB GR_WNT              ; (while our claim is out the cursor waits
        CI NPLOTS-1             ; on its plot)
        BC GS7
        LDB GR_SRV
        LR 4,A                  ; k
        LDB G_CUR
        LR 5,A
        XS 4
        BZ GS6
        LR A,5                  ; we made the step it waits for a little
        CI NPLOTS-1             ; early: no jump back (after a claim it
        BNC GS5                 ; waits longer, and the cursor goes there)
        LR A,4
        CI 0FFH
        BZ GSA
        COM
        AS 5                    ; cur + ~k carries: k < cur
        BNC GS5
GSA:    LR A,0
        NI 0FFH
        BZ GS5
        LDB G_STEP
        LR 6,A
        COM
        AS 0                    ; w + ~step carries: w > step
        BC GS5
        LR A,6
        AS 0
        LR 0,A                  ; w + step
        BR GS6
GS5:    LI 0FFH                 ; a claim (ours, or another's the cursor
        STB GR_WAS              ; went back for)
        LR A,4
        STB G_CUR
GS6:    LR A,0
        STB G_NEXT
GS7:    LDB ST+H_STOP           ; no further than where a computer stops it
        NI 0FFH                 ; (an older server doesn't say: one plot
        BZ GS8                  ; past its cursor)
        AI 0FFH
        BR GS9
GS8:    LDB GR_SRV
        INC
GS9:    STB GR_LIM
        LDB GR_DEP              ; the server's cursor has started: our ship
        NI 0FFH                 ; has gone too
        BZ GSS
        LDB GR_SRV
        CI NPLOTS-1
        BNC GSX
        PI GRDDONE
GSS:    LIS 1
        STB G_SHOW
GSX:    LEAVE

; GRCLAIM -- our claim's answer (GR_SRV: the server's cursor).
GRCLAIM: ENTER
        LDB MYSEAT
        CI 3
        JNC GCX
        DCI ST+GR_CLM
        ADC
        LM
        LR 6,A                  ; mine
        CI NPLOTS-1
        BNC GC10
        LIS 1
        STB GR_CLD
        LI 0FFH
        STB GR_WNT
GC10:   LDB GR_WNT
        LR 7,A                  ; j: the plot we wait on
        CI NPLOTS-1
        JNC GCX
        LR A,6
        CI NPLOTS-1
        JC GCX
        LDB ACTSENT             ; only the state answering the claim judges
        CI ACT_CLAIM            ; it: those before were made before the
        BNZ GC12                ; server saw it
        LDB ST+H_ERR
        NI 0FFH
        BZ GCDN
        LDB GR_SRV              ; turned down as the server's cursor wasn't
        CI 0FFH                 ; there yet (k < j): send it again when it
        BZ GC11                 ; is; otherwise press again
        COM
        AS 7                    ; j + ~k carries: k < j
        BNC GCDN
GC11:   LIS 1
        STB GR_PRK
        BR GCX
GC12:   LDB GR_PRK
        NI 0FFH
        BZ GCX
        LDB ACT
        NI 0FFH
        BNZ GCX
        LDB GR_SRV              ; it's time once k >= j
        CI 0FFH
        BZ GCX
        COM
        AS 7                    ; j + ~k carries: k < j
        BC GCX
        CLR
        STB GR_PRK
        LDB GR_SRV
        CI NPLOTS-1
        BNC GCDN
        LIS ACT_CLAIM
        LR 0,A
        LR A,7
        LR 1,A
        CLR
        LR 2,A
        PI ACTSEND
        BR GCX
GCDN:   LI 0FFH                 ; press again
        STB GR_WNT
        CLR
        STB GR_CLD
        STB GR_PRK
        LIS SND_ERROR
        LR 0,A
        PI SNDPLAY
GCX:    LEAVE

; ---------------------------------------------------------------------------
; GRANTISR -- every frame: the cursor steps to the next free plot when its
;   countdown runs out, blanking for that frame, like the Atari.
; ---------------------------------------------------------------------------
GRANTISR: ENTER
        LDB G_SHOW              ; the press: the plot on screen then
        NI 0FFH
        BZ GI2
        LDB G_CUR
        CI NPLOTS-1
        BNC GI2
        LDB EVBTN
        NI 0FFH
        BZ GI2
        LDB GR_PRS
        CI 0FFH
        BNZ GI2
        LDB G_CUR
        STB GR_PRS
        LDB G_NEXT              ; the frame after a step: the plot before
        LR 4,A
        LDB G_STEP
        XS 4
        BNZ GI2
        LDB GR_WAS
        CI NPLOTS-1
        BNC GI2
        STB GR_PRS
GI2:    LDB GR_WNT              ; our claim is out: it waits on its plot
        CI NPLOTS-1
        BC GI4
        LDB G_NEXT
        NI 0FFH
        BZ GI4
        AI 0FFH
        STB G_NEXT
        NI 0FFH
        BNZ GI4
        LDB G_CUR
        CI NPLOTS
        BZ GI4
        LR 5,A
        PI NEXTFR
        LR 6,A                  ; the next free plot
        LDB GR_LIM              ; a computer stops it before that: the
        COM                     ; state says when
        AS 6                    ; n + ~lim carries: n > lim
        BNC GI3
        LIS 1
        STB G_NEXT
        BR GI4
GI3:    LDB G_CUR               ; a step
        STB GR_WAS
        LR A,6
        STB G_CUR
        LDB G_STEP
        STB G_NEXT
        BR GIH
GI4:    LDB G_CUR
        CI NPLOTS-1
        BNC GIH
        LDB G_SHOW
        NI 0FFH
        BNZ GI5
GIH:    LIS SLOT_CUR
        LR 0,A
        PI SPRHIDE
        LEAVE
GI5:    LDB G_CUR
        LR 0,A
        PI PLATXY
        LIS SLOT_CUR
        STB SPA_SL
        LIS SH_CURSOR
        STB SPA_SH
        CLR
        STB SPA_FR
        STB SPA_FL
        STW 4, 5, SPA_XL
        STW 6, 7, SPA_YL
        LIS C_WHITE
        STB SPA_CO
        PI SPRSET
        LEAVE

; ---------------------------------------------------------------------------
; GRANTT -- grant_tick: the ship, and the button's claim.
; ---------------------------------------------------------------------------
GRANTT: ENTER
        PI MSGTICK
        LDB GR_DEP
        NI 0FFH
        BZ GT1
        LDB FXFLAG
        CI 1
        BZ GT0
        LDB FXA+AC_ST
        NI 0FFH
        BNZ GTX
GT0:    PI GRDDONE
GTX:    LEAVE
GT1:    LDB EVBTN
        NI 0FFH
        BZ GTX
        LDB GR_PRS              ; the plot the cursor was on at the press
        LR 4,A
        CI NPLOTS-1
        BC GT2
        LDB G_CUR
        LR 4,A
GT2:    LI 0FFH
        STB GR_PRS
        LDB GR_CLD
        NI 0FFH
        BNZ GTX
        LDB MYSEAT
        CI 3
        BNC GTX
        LR A,4
        CI NPLOTS-1
        BNC GTX
        LIS ACT_CLAIM
        LR 0,A
        LR A,4
        LR 1,A
        CLR
        LR 2,A
        PI ACTSEND
        LIS 1                   ; hold the cursor there until the server
        STB GR_CLD              ; answers
        LR A,4
        STB GR_WNT
        STB G_CUR
        LI 0FFH
        STB GR_WAS
        LIS SND_BUY
        LR 0,A
        PI SNDPLAY
        LEAVE

; ---------------------------------------------------------------------------
; The production phase (events.c): ProductionDisplay, 1421 jiffies on the
; server. A colony event plays its set piece first, then the dots.
; ---------------------------------------------------------------------------

; PRODE -- production_enter.
PRODE:  ENTER
        CLR
        STB MAPDOTS
        PI MAPENTER
        LDB ST+C_EVENT
        STB PR_EV
        LDB ST+C_PLOT
        CI NPLOTS-1
        BC PE1
        CLR
PE1:    STB PR_PL
        CLR
        STB PR_STR
        STB PR_DON
        STB FXFLAG
        STB PR_LAST
        STB PR_LAST+1
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        LDB PR_EV               ; 1..9: a colony event
        AI 0FFH
        CI 8
        BNC PE2
        AI TI_EVENT
        LR 0,A
        CLR
        LR 1,A
        PI TITLE
        LDB PR_EV
        AI MS_EVENT-1
        LR 0,A
        PI GMSG
        PI PRSTART
        LEAVE
PE2:    PI PRTITLE
        LIS 1
        STB PR_DON
        LEAVE

; PRTITLE -- "PRODUCTION" with the round.
PRTITLE: ENTER
        LI TI_PRODUCTION
        LR 0,A
        LDB RND
        LR 1,A
        PI TITLE
        LEAVE

; PRSTART -- the event's set piece (pr_event_start).
PRSTART: ENTER
        LDB PR_EV
        CI CE_PIRAT
        BNZ PS1
        DCI FXPIRATE
        JMP PSGO
PS1:    CI CE_SHIP
        BNZ PS2
        DCI FXLAND
        JMP PSGO
PS2:    CI CE_PEST              ; crawling round its plot
        BNZ PS3
        LDB PR_PL
        LR 0,A
        PI PLATXY
        STW 4, 5, FXA+AC_XL
        STW 6, 7, FXA+AC_YL
        DCI FXPEST
        JMP PSGO
PS3:    CI CE_ACID              ; along the plot's row, lightning under it
        BNZ PS4
        LDB PR_PL
        LR 0,A
        PI PLATXY
        CLR
        LR 4,A
        LR 5,A
        STW 4, 5, FXA+AC_XL
        STW 6, 7, FXA+AC_YL
        STW 4, 5, FXA+FXSZ+AC_XL
        LI 24
        AS 6
        LR 6,A
        STW 6, 7, FXA+FXSZ+AC_YL
        LIS 1
        LR 0,A
        DCI FXBOLT
        LR H,DC
        PI FXSTART
        DCI FXCLOUD
        JMP PSGO
PS4:    CI CE_SUN
        BNZ PS5
        LIS 1
        LR 0,A
        DCI FXSPARK2
        LR H,DC
        PI FXSTART
        DCI FXSPARKS
        JMP PSGO
PS5:    CI CE_RADIA             ; the plot's M.U.L.E.
        BNZ PS6
        LDB PR_PL
        LR 0,A
        PI PLATXY
        LR A,4
        AI 8
        LR 4,A
        LR A,6
        AI 8
        LR 6,A
        STW 4, 5, FXA+AC_XL
        STW 6, 7, FXA+AC_YL
        DCI FXRADIA
        JMP PSGO
PS6:    CI CE_METEO             ; from 120 right (at most 182), 120 up
        BNZ PS7
        LDB PR_PL
        LR 0,A
        PI PLATXY
        LR A,4
        CI 62
        LI 182
        BNC PS6A
        LR A,4
        AI 120
PS6A:   LR 4,A
        LR A,6
        AI 136                  ; y - 120: carries when y >= 120
        LR 6,A
        LIS 0
        BC PS6B
        LI 0FFH
PS6B:   LR 7,A
        STW 4, 5, FXA+AC_XL
        STW 6, 7, FXA+AC_YL
        DCI FXMETEOR
        JMP PSGO
PS7:    CI CE_FIRE              ; fire and quake are drawn by PRODT
        BZ PS8
        CI CE_QUAKE
        BNZ PSX
PS8:    LDW 0, 1, FRAMES
        STW 0, 1, PHT0
        BR PSX
PSGO:   LR H,DC                 ; actor 0 runs the script at DC0
        CLR
        LR 0,A
        PI FXSTART
PSX:    LIS 1
        STB PR_STR
        LIS SND_EVENT
        LR 0,A
        PI SNDPLAY
        LEAVE

; PRODT -- production_tick.
PRODT:  ENTER
        PI MSGTICK
        LDB PR_DON
        NI 0FFH
        BZ PT1
        LDB MAPDOTS             ; the production dots
        NI 0FFH
        BNZ PTX
        LIS 1
        STB MAPDOTS
        PI MAPALL
PTX:    LEAVE
PT1:    LDB PR_EV
        CI CE_FIRE
        JNZ PT5
        LI 640&0FFH             ; the store burns for 640 frames
        LR 2,A
        LI 640>>8
        LR 3,A
        PI PHAGE
        NI 0FFH
        JNZ PTDONE
        PI PRFLICK
        NI 0FFH
        BZ PTX
        PI RAND                 ; fire flickers over it, a quarter at a time
        STB PR_K
        NI 1
        LIS 0
        BZ PF1
        LIS 5
PF1:    AI 46                   ; the store's plot is at (46, 26)
        STB MP_X
        LDB PR_K
        NI 2
        SL 1
        AI 26
        STB MP_Y
        LDB PR_K
        SR 1
        SR 1
        NI 1
        AI PICN_FIRE
        LR 2,A
        LDB PR_K
        NI 8
        LIS C_YELLOW
        BZ PF2
        LIS C_ORANGE
PF2:    LR 0,A
        PI PICAT
        LDB PR_K
        NI 030H
        BNZ PTX
        LI STOREPL
        LR 0,A
        PI MAPPLOT
        BR PTX
PT5:    LDB PR_EV               ; (JNZ's JMP lost A)
        CI CE_QUAKE             ; plots jolt (flash outlined) for 300 frames
        JNZ PT9
        LI 300&0FFH
        LR 2,A
        LI 300>>8
        LR 3,A
        PI PHAGE
        NI 0FFH
        BNZ PT7
        PI PRFLICK
        NI 0FFH
        JZ PTX
        PI QKOFF
        PI RAND
PQ1:    CI NPLOTS-1
        BC PQ2
        AI 256-NPLOTS
        BR PQ1
PQ2:    STB MAPBLNK
        LR 0,A
        LIS 1
        STB MAPBON
        PI MAPPLOT
        JMP PTX
PT7:    PI QKOFF
        BR PTDONE
PT9:    LDB FXFLAG              ; the others: over when the set piece is
        CI 1
        BZ PTDONE
        LDB PR_STR
        NI 0FFH
        JZ PTX
        DCI FXA+AC_ST
        LM
        DCI FXA+FXSZ+AC_ST
        OM
        JNZ PTX
PTDONE: LIS 1
        STB PR_DON
        PI FXSTOP
        PI PRTITLE
        LIS 1
        STB MAPDOTS
        PI MAPALL
        JMP PTX

; PRFLICK -- A = 1 (and PR_LAST = now) if 4 frames have passed since
;   PR_LAST, else 0.
PRFLICK: ENTER
        LDW 0, 1, FRAMES
        LDW 2, 3, PR_LAST
        PI SUB16
        LIS 4
        LR 2,A
        CLR
        LR 3,A
        PI GE16
        LIS 0
        BNC PFL1
        LDW 0, 1, FRAMES
        STW 0, 1, PR_LAST
        LIS 1
PFL1:   LEAVE

; QKOFF -- the plot outlined by the quake drawn plain again.
QKOFF:  ENTER
        LDB MAPBLNK
        CI NPLOTS-1
        BNC QK1
        LR 0,A
        LI NOBODY
        STB MAPBLNK
        PI MAPPLOT
QK1:    LEAVE
