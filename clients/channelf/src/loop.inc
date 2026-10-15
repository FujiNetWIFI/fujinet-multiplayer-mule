; ---------------------------------------------------------------------------
; loop.inc -- the frame loop and a state's arrival (mekkogx main.c).
;
; Each frame: wait for the cart's tick to move, put the sprites up, sound,
; input; then what moves by itself, once for every frame that has passed
; (catching up after a slow frame); one step of the network request; then
; the phase's per-frame work. The phase's code is in its page, mapped when a
; state arrives (PGOF); the tables below hold its entries' addresses, which
; all point into the page window.
; ---------------------------------------------------------------------------

SCR_NONE EQU 0
SCR_TEXT EQU 1
SCR_MAP  EQU 2
SCR_TOWN EQU 3
SCR_AUC  EQU 4
SCR_SUM  EQU 5
SCR_FOUN EQU 6

AN_NONE  EQU 0
AN_WALK  EQU 1
AN_GRANT EQU 2
AN_AUC   EQU 3

DV_WAIT  EQU 0
DV_PROMPT EQU 1
DV_WALK  EQU 2
DV_OVER  EQU 3

; the screen's bands: the title row and the message rows black (white
; text), the rest light grey (blue, red and green on it)
MAPY    EQU 10                  ; the map's first row
MSGY    EQU 50                  ; the message's first row

; phase -> page, and its entries
PGOF:   DB PGSTAT, PGGRANT, PGAUC, PGDEV, PGGRANT, PGAUC, PGAUC, PGSTAT, PGSTAT
ENTERS: DW LOBBYE, GRANTE, LAUCE, DEVE, PRODE, GOODSE, GOODSE, SUME, OVERE
STATES: DW LOBBYS, GRANTS, AUCS, DEVS, STUB, AUCS, AUCS, SUMS, STUB
TICKS:  DW LOBBYT, GRANTT, AUCT, DEVT, PRODT, AUCT, AUCT, SUMT, OVERT

; CALLPH -- call this phase's entry from table tab (an ENTER'd caller).
CALLPH  MACRO tab
        LDB PHASE
        SL 1
        DCI tab
        ADC
        LM
        LR KU,A
        LM
        LR KL,A
        PK
        ENDM

; ---------------------------------------------------------------------------
; BANDS -- the screen's palettes: the title band and the message band black,
;   between them light grey.
; ---------------------------------------------------------------------------
BANDS:  ENTER
        LI MAPY
        LR 4,A
        LI MSGY-1
        LR 5,A
        PI BANDS3
        LEAVE

; BANDS3 -- rows r4..r5 light grey (palette 2), the rest of the screen black
;   (palette 0).  Clobbers A, r0-r9, H, DC0.
BANDS3: ENTER
        LR A,4
        STB BND_A
        LR A,5
        STB BND_B
        LI VY0
        LR 1,A
        LDB BND_A
        AI 0FFH
        LR 2,A
        CLR
        LR 3,A
        PI DPAL
        LDB BND_A
        LR 1,A
        LDB BND_B
        LR 2,A
        LIS 2
        LR 3,A
        PI DPAL
        LDB BND_B
        INC
        LR 1,A
        LI VY1
        LR 2,A
        CLR
        LR 3,A
        PI DPAL
        LEAVE

; CATCHN -- NCATCH = the animation steps to run now, once a frame gone by:
;   more than 2 behind, half now and the rest after; more than 8, the rest
;   let go (counted in NSATUR). DONE moves on by them.
CATCHN: ENTER
        LDW 0, 1, FRAMES
        LDW 2, 3, DONE
        PI SUB16                ; n = FRAMES - DONE
        LR A,1
        NI 0FFH
        BNZ CN2
        LR A,0
        CI 8
        BC CN3
CN2:    LDW 0, 1, FRAMES        ; DONE = FRAMES - 8, n = 8
        LIS 8
        LR 2,A
        CLR
        LR 3,A
        PI SUB16
        STW 0, 1, DONE
        INCB NSATUR
        LIS 8
        LR 0,A
CN3:    LR A,0
        CI 2
        BC CN4
        INC
        SR 1
        LR 0,A
CN4:    LR A,0                  ; DONE += n
        STB NCATCH
        LR 2,A
        CLR
        LR 3,A
        LDW 0, 1, DONE
        PI ADD16
        STW 0, 1, DONE
        LEAVE

; PENOW -- r0:r1 = the phase's elapsed jiffies now (mule.h pe_now).
;   Clobbers A, r2, r3, DC0.
PENOW:  ENTER
        LDW 0, 1, FRAMES
        LDW 2, 3, RXFRAME
        PI SUB16
        LDW 2, 3, RXPE
        PI ADD16
        LEAVE

; PHAGE -- A = 1 if FRAMES - PHT0 >= r2:r3, else 0 (the phase's own
;   animation that long under way).  Clobbers r0-r3, r6, r7, DC0.
PHAGE:  ENTER
        LR A,2
        LR 6,A
        LR A,3
        LR 7,A
        LDW 0, 1, FRAMES
        LDW 2, 3, PHT0
        PI SUB16
        LR A,6
        LR 2,A
        LR A,7
        LR 3,A
        PI GE16
        LIS 0
        BNC PHA1
        LIS 1
PHA1:   LEAVE

; ---------------------------------------------------------------------------
; MAIN -- from START: the title, the test hook, boot, then the loop forever.
; ---------------------------------------------------------------------------
MAIN:   PI SNDINIT
        PI BANDS
        LI 28                   ; "M.U.L.E." / "TOURNAMENT"
        LR 2,A
        LIS C_ORANGE
        LR 6,A
        DCI STW
        LI WD_MULE
        LR 0,A
        PI STRP
        PI TCENTER
        LI 34
        LR 2,A
        LIS C_BLUE
        LR 6,A
        DCI STW
        LI WD_TOURNAMENT
        LR 0,A
        PI STRP
        PI TCENTER
        LI 03CH                 ; the harness may poke now
        STB TESTHK
        LI 0C3H
        ST
        LI 45
        STB GT
MNT1:   PI FRWAIT
        PI SNDTICK
        LDB TESTHK
        CI 05AH
        BZ MNTEST
        DECB GT
        BNZ MNT1
        LIS PGBOOT
        LR 9,A
        PI SELPG
        PI BOOT
        BR MNGO
MNTEST: LI 0A5H                 ; tell the harness
        STB TESTHK
        LIS PGBOOT
        LR 9,A
        PI SELPG
        PI TESTST
MNGO:   PI GAMESTRT
        LDW 0, 1, FRAMES
        STW 0, 1, DONE

MAINLP: LISU 7                  ; (the K-stack is empty out here)
        LISL 3
        LI KSTK0
        LR S,A
        LIS 1
        STB DBGAT
        PI FRWAIT
        LIS 2
        STB DBGAT
        PI SPRFLUSH
        LIS 3
        STB DBGAT
        PI SNDTICK
        PI INPOLL
        PI INTAKE
        ; watching (no seat): only the leave key counts
        LDB STOK
        NI 0FFH
        BZ ML1
        LDB MYSEAT
        CI 3
        BC ML1
        CLR
        STB INPDIR
        STB INPBTN
        STB EVBTN
        LDB EVKEY
        CI KEY_LEAVE
        BZ ML1
        CLR
        STB EVKEY
ML1:
        IF AUTOPLAY
        FAR PGBOOT, AUTOPL
        ENDIF
        LIS 4
        STB DBGAT
        PI CATCHN
MLAN:   LDB NCATCH
        NI 0FFH
        BZ MLNET
        AI 0FFH
        STB NCATCH
        LDB ANIM
        CI AN_WALK
        BNZ MLA2
        PI WALKISR
        BR MLA4
MLA2:   CI AN_GRANT
        BNZ MLA3
        PI GRANTISR
        BR MLA4
MLA3:   CI AN_AUC
        BNZ MLA4
        PI AUCISR
MLA4:   PI FXRUN
        BR MLAN
MLNET:  LIS 6
        STB DBGAT
        ; the network: one step a frame
        PI NETBUSY
        BZ MLIDLE
        PI NETSTEP
        NI 0FFH
        BZ MLKEY
        PI REPLY
        BR MLKEY
MLIDLE: LDB POLLW
        NI 0FFH
        BZ MLI1
        AI 0FFH
        STB POLLW
MLI1:   LDB POLLW
        NI 0FFH
        BZ MLGO
        LDB ACT
        NI 0FFH
        BZ MLKEY
MLGO:   LDB TESTMD
        NI 0FFH
        BZ MLREAL
        PI TESTPOLL             ; (resident: STARRIV maps the page)
        PI POLLRATE
        BR MLKEY
MLREAL: PI NETBEGIN
MLKEY:  LIS 7
        STB DBGAT
        ; the leave key, in a game: ask first (a spectator just goes)
        LDB STOK
        NI 0FFH
        BZ MLTK
        LDB PHASE
        NI 0FFH
        BZ MLTK
        LDB EVKEY
        CI KEY_LEAVE
        BNZ MLTK
        LDB MYSEAT
        CI 3
        BC MLASK
        LDB TESTMD
        NI 0FFH
        BNZ MLASK
        PI LEAVETB
        BR MLRE
MLASK:  PI LEAVEASK
MLRE:   LDW 0, 1, FRAMES
        STW 0, 1, DONE
        JMP MAINLP
MLTK:   LDB STOK
        NI 0FFH
        BZ MLSP
        CALLPH TICKS
MLSP:   ; the spectator's tag, every 32 frames (a repaint takes it off)
        LDB STOK
        NI 0FFH
        BZ MLEND
        LDB MYSEAT
        CI 3
        BC MLEND
        LDW 0, 1, FRAMES        ; (FRAMES >> 5) & $FF
        LR A,0
        SR 4
        SR 1
        LR 2,A
        LR A,1
        SL 1
        SL 1
        SL 1
        AS 2
        LR 2,A
        DCI SPECAT
        CM
        BZ MLEND
        LR A,2
        STB SPECAT
        PI SPECTAG
MLEND:  JMP MAINLP

; ---------------------------------------------------------------------------
; STARRIV -- a whole state is in ST (state_arrived).
; ---------------------------------------------------------------------------
STARRIV: ENTER
        LDB ST+H_PHASE
        CI PH_OVER
        BC SAR1
        LEAVE
SAR1:   PI CLKSYNC
        LDB ST+H_PHASE
        STB PHASE
        LDB ST+H_ROUND
        STB RND
        LDB ST+H_SEAT
        STB MYSEAT
        LDW 0, 1, ST+H_ELAP
        STW 0, 1, RXPE
        LDW 0, 1, FRAMES        ; rx_frame = frames - rx_age
        LDB RXAGE
        LR 2,A
        CLR
        LR 3,A
        PI SUB16
        STW 0, 1, RXFRAME
        LIS 1
        STB STOK
        LDB PHASE               ; the phase's page
        DCI PGOF
        ADC
        LM
        LR 9,A
        PI SELPG
        LDB PHASE               ; a new phase (or a new round's grant): enter
        LR 0,A
        DCI LASTPH
        CM
        BNZ SAREN
        LR A,0
        CI PH_GRANT
        BNZ SARST
        LDB RND
        DCI LASTRND
        CM
        BZ SARST
SAREN:  CLR
        STB MSGSCR
        STB MSGONE
        PI FXSTOP
        CLR
        STB ANIM
        CALLPH ENTERS
SARST:  CALLPH STATES
        DCI ST                  ; prev = st
        XDC
        DCI PREV
        XDC
        LI STSIZE&0FFH
        LR 0,A
        LI STSIZE>>8
        LR 1,A
        PI MEMCPY
        LDB PHASE
        STB LASTPH
        LDB RND
        STB LASTRND
        LEAVE

; ---------------------------------------------------------------------------
; CLKSYNC -- clock_sync. The server's clock keeps ours honest; ours never
;   misses a frame (the cart's tick), so a gap larger than any before is
;   only a faster reply, and the new base. A reply slower than the fastest
;   is that much older than it looks: RXAGE says by how much (at most 30).
; ---------------------------------------------------------------------------
CLKSYNC: ENTER
        CLR
        STB RXAGE
        LDB TESTMD
        NI 0FFH
        BNZ CS0
        LDB ST+H_PHASE
        NI 0FFH
        BNZ CS1
CS0:    CLR
        STB CSHAVE
        LEAVE
CS1:    LDW 0, 1, ST+H_T        ; lag = st.h.t - frames
        LDW 2, 3, FRAMES
        PI SUB16
        STW 0, 1, GT
        LDB CSHAVE
        NI 0FFH
        BNZ CS2
        LIS 1
        STB CSHAVE
        BR CSBASE1
CS2:    LDW 2, 3, CSBASE        ; d = lag - base
        PI SUB16
        LR A,0
        OI 0
        BNZ CS3
        LR A,1
        NI 0FFH
        BZ CSX                  ; the same: nothing to do
CS3:    LR A,1
        NI 080H
        BZ CSBASE1              ; later than the base: the new base
        LDW 0, 1, CSBASE        ; older than the fastest: by how much?
        LDW 2, 3, GT
        PI SUB16
        LR A,1
        NI 0FFH
        BNZ CSBASE1             ; much older: the server's clock went back
        LR A,0
        CI 120
        BNC CSBASE1
        CI 30
        BC CS4
        LI 30
CS4:    STB RXAGE
        LEAVE
CSBASE1: LDW 0, 1, GT
        STW 0, 1, CSBASE
CSX:    LEAVE

; ---------------------------------------------------------------------------
; REPLY -- a request is over (main.c reply).
; ---------------------------------------------------------------------------
REPLY:  ENTER
        LDB NETHAVE
        CI 1
        BNZ RP2
        CLR
        STB NETFAIL
        PI STARRIV
        BR RPX
RP2:    CI 2
        BNZ RP3
        LDB ST+H_ERR            ; refused outright: another table
        CI ERR_TFULL
        BZ RPT
        CI ERR_SEAT
        BNZ RPX
RPT:    PI CHOOSETB
        LI 0FFH
        STB LASTPH
        CLR
        STB STOK
        BR RPX
RP3:    INCB NETFAIL
RPX:    PI POLLRATE
        LEAVE

; POLLRATE -- how often to ask: often while things move, rarely while we
;   walk (the server has nothing to tell us then) or wait.
POLLRATE: ENTER
        LI 30
        LR 0,A
        LDB NETFAIL
        NI 0FFH
        BNZ PRX
        LDB PHASE
        CI PH_GRANT
        BZ PR12
        CI PH_LAUC
        BZ PR12
        CI PH_DECL
        BZ PR12
        CI PH_TRADE
        BZ PR8
        CI PH_PROD
        BZ PR60
        CI PH_OVER
        BZ PR60
        CI PH_DEV
        BNZ PRX
        LDB DVSTATE
        CI DV_WALK
        BNZ PRX
PR60:   LI 60
        BR PRS
PR12:   LI 12
        BR PRS
PR8:    LIS 8
PRS:    LR 0,A
PRX:    LR A,0
        STB POLLW
        LEAVE

; CHOOSETB -- another table: the room forgotten, the list, the choice kept.
;   All of that is in the boot page.
CHOOSETB: ENTER
        FAR PGBOOT, TBRECH
        LEAVE

; GAMESTRT -- joining (game_start).
GAMESTRT: ENTER
        LI 0FFH
        STB LASTPH
        STB LASTRND
        CLR
        STB SCRNOW
        STB STOK
        STB POLLW
        STB ACT
        PI DCLS
        LI 28
        LR 2,A
        LIS C_WHITE
        LR 6,A
        DCI S_JOIN
        PI TCENTER
        LEAVE
S_JOIN: DB "JOINING...",0

; LEAVETB -- leave the table (the server gives the computer our seat) and
;   pick another.
LEAVETB: ENTER
        CLR
        STB ANIM
        PI FXSTOP
        PI SPRHALL
        LI ACT_LEAVE
        STB ACT
        PI NSYNC
        PI CHOOSETB
        PI GAMESTRT
        LEAVE

; LEAVEASK -- the leave key in a game: ask first. The game stands still while
;   we ask; the key again leaves, anything else carries on.
LEAVEASK: ENTER
        LI MSGY
        STB MSGROW
        LIS C_WHITE
        STB MSGCOL
        CLR
        STB EXPV1
        ST
        STB EXPV2
        ST
        LI MS_LEAVEASK
        LR 0,A
        PI MSGSHOW
LVA1:   PI FRWAIT
        PI SPRFLUSH
        PI SNDTICK
        PI MSGTICK
        PI INPOLL
        PI INTAKE
        LDB EVBTN
        NI 0FFH
        BNZ LVA2
        LDB EVKEY
        NI 0FFH
        BZ LVA1
LVA2:   LDB EVKEY
        CI KEY_LEAVE
        BNZ LVA3
        LDB TESTMD
        NI 0FFH
        BNZ LVA3
        PI LEAVETB
        LEAVE
LVA3:   PI MSGCLR
        CLR
        STB EVBTN
        ST
        LEAVE
