; ram.inc -- the RAM ledger.
;
; The console has 4K of RAM, 4000H-4FFFH, and the screen is most of it:
; LINES lines of 40 bytes from 4000H. Everything above the visible screen is
; ours, up to the IM 2 vector and the stack under the BIOS cells at 4FC0H.
; tools/checksize.py fails the build if the variables run into the stack.
;
; VAR allocates upward from RAMBASE. The PHASE block at the end is shared:
; each phase page lays its own variables over it (PV), because only one
; phase runs at a time.
;
; The state: the C client keeps three 550-byte copies (st, prev, rx). Here
; the reply stays where the cart painted it (the 1K window at 3B00H, read
; with RXGETB) from its READ until the next request begins, and a compact
; HOT copy -- the header, the players, the map's terrain/owner/M.U.L.E. and
; the phase's own section -- is what the per-frame code reads. When a new
; state arrives the phase code compares the window with HOT (st with prev),
; then HOT is refreshed.

LINES   EQU     78              ; 13 text rows of 6 lines
RAMBASE EQU     4000H+LINES*40  ; 4C30H

STACK   EQU     4FC0H           ; grows down; 4FC0H+ belongs to the BIOS
STKLOW  EQU     4F70H           ; 80 bytes of stack
VECTOR  EQU     4F6EH           ; the IM 2 vector word (I = 4FH, INFBK = 6EH)
SCRATCH EQU     4F6DH           ; a byte clipped sprite writes may XOR

RAMP    DEFL    RAMBASE
VAR     MACRO   nm, sz
nm      EQU     RAMP
RAMP    DEFL    RAMP+sz
        ENDM

; ---- the clock and paging ----------------------------------------------
        VAR     FRAMES,2        ; 60 Hz, counted by the interrupt
        VAR     DONE,2          ; frames the loop has caught up with
        VAR     CURPG,1         ; the page mapped at 2000H
        VAR     FCHL,2          ; FARCALL scratch
        VAR     FCTGT,2
        VAR     FCA,1
        VAR     RNDS,2          ; random seed
        VAR     TESTHK,4        ; the harness's hook (see BOOT)
        VAR     DBGAT,1         ; where the loop is (the harness peeks it)
        VAR     APLAST,2        ; autoplay's last press
        VAR     FXON,1          ; the actors (fx.inc) are running
        VAR     FXFLAG,1        ; a script's FX_SIG
        VAR     TPH,1           ; the offline test mode's state (test.inc):
        VAR     TARG,1          ;   phase, event/stage/seat, player event
        VAR     TPEV,1
        VAR     TERR,1
        VAR     TELAP,2
        VAR     TRES,2
        VAR     TCUR,1          ;   the grant's cursor
        VAR     TNEXT,2
        VAR     TCLM,1
        VAR     TFL,1           ;   the turn
        VAR     TCARRY,1
        VAR     TLEFT,2
        VAR     TTEL,2
        VAR     TNP,1           ;   plots changed: {plot, owner, mule} x 4
        VAR     TPAT,12
        VAR     TSW,2           ;   frames since the last (made-up) state

; ---- the network --------------------------------------------------------
        VAR     CURSLC,1        ; reply slice the cart is publishing
        VAR     AVAIL,2         ; NET_STATUS bytes waiting
        VAR     PRVAVL,2        ; the settle loop's previous reading
        VAR     RXLEN,2         ; reply length, captured after a READ
        VAR     NSTATE,1        ; request: 0 idle, 1 status, 2 read
        VAR     NWAIT,1         ; frames without progress
        VAR     NETHAVE,1       ; 1 a state, 2 an error header, 0 failed
        VAR     NETFAIL,1
        VAR     POLLW,1         ; frames to the next poll
        VAR     ACT,1           ; queued action (ACT_*)
        VAR     ACTARG,2
        VAR     ACTSNT,1        ; the action the last request carried
        VAR     AUTRADE,1       ; units traded as shown (pushes carry it)
        VAR     STVALID,1       ; 1: the window holds the last state
        VAR     TESTMD,1        ; offline test mode
        VAR     CSBASE,2        ; clock_sync
        VAR     CSHAVE,1
        VAR     URLREQ,1        ; the request being composed

; ---- who and where --------------------------------------------------------
        VAR     SCNAME,9        ; player name, NUL-terminated
        VAR     SCTABL,9        ; table id
ENDPTN  EQU     64
        VAR     ENDPT,ENDPTN+1  ; server URL (no trailing '/')

; ---- the game ---------------------------------------------------------------
        VAR     PHS,1  
        VAR     RND,1
        VAR     MYSEAT,1
        VAR     LASTPH,1
        VAR     LASTRND,1
        VAR     STOK,1
        VAR     NEWPH,1         ; the state that arrived began a phase
        VAR     RXPE,2          ; phase-elapsed at the last state ...
        VAR     RXFRM,2         ; ... and the frame it came
        VAR     PHT0,2          ; a phase's own animation began
        VAR     SCRNOW,1
        VAR     ANIM,1
        VAR     DVSTATE,1       ; develop's state (the poll rate needs it)

; ---- HOT: the state the per-frame code reads (see above) --------------------
HOTSZ   EQU     16+48+135+58
        VAR     HOT,HOTSZ
HOTH    EQU     HOT             ; the header, 16 bytes
HOTP    EQU     HOT+16          ; 4 x 12: player bytes 9..20 (species ..
HPLSZ   EQU     12              ;   rank; see PL_* in win.inc)
HMAP    EQU     HOTP+48         ; the map's first 135 bytes:
HTERR   EQU     HMAP            ;   terrain[45]
HOWNER  EQU     HMAP+45         ;   owner[45]
HMULE   EQU     HMAP+90         ;   mule[45] (crystite and production are
                                ;   read from the window, STVALID)
HOTU    EQU     HMAP+135        ; the phase's section: grant (8), auction
HOTUSZ  EQU     58              ;   (58), turn (its first 14; develop
                                ;   copies the wampus list itself)

; ---- drawing's scratch (the sprite and map pages) ----------------------------
        VAR     SPA_SH,1        ; SPRSET's arguments, in this order
        VAR     SPA_FR,1
        VAR     SPA_X,2
        VAR     SPA_Y,2         ; Atari lines
        VAR     SPA_COL,1
        VAR     SPA_FL,1
        VAR     PLK_ST,1        ; the plot's look (gfx.h PlotLook)
        VAR     PLK_RIV,1
        VAR     PLK_MNT,1
        VAR     PLK_OWN,1
        VAR     PLK_ICON,1
        VAR     PLK_DOTS,1
        VAR     PLK_COL,1
        VAR     GTMP,22         ; one drawing at a time: shared by
PBA     EQU     GTMP            ; BLIT: a sprite's old place (sprpg.inc PB_*)
PBB     EQU     GTMP+10         ;   and its new one
BROW    EQU     GTMP+20         ;   the line being written
PLADR   EQU     GTMP            ; XPLOT
PLOWN   EQU     GTMP+2
PLICN   EQU     GTMP+4
PLWH    EQU     GTMP+6
PLRV    EQU     GTMP+8
PLDOTS  EQU     GTMP+10
PLX1    EQU     GTMP+11
PLX2    EQU     GTMP+12
PLLN    EQU     GTMP+13
PLM0    EQU     GTMP+14
PLM1    EQU     GTMP+16
PLM2    EQU     GTMP+18
XTN     EQU     GTMP            ; XTOWN
XTV     EQU     GTMP+1
XFV2    EQU     GTMP            ; XFILL

; ---- input ----------------------------------------------------------------
        VAR     INDIR,1         ; DIR_* level
        VAR     INBTN,1         ; button level
        VAR     EVBTN,1         ; taken: pressed
        VAR     EVKEY,1         ; taken: a key
        VAR     LBTN,1          ; latches
        VAR     LKEY,1
        VAR     KPPREV,1        ; last raw key (edges)
        VAR     BTPREV,1        ; last button level
        VAR     INPHK,1         ; test hook: nonzero -> the three below
        VAR     INPHKD,1
        VAR     INPHKB,1
        VAR     INPHKK,1
        VAR     SPAT,1          ; the SPECTATOR tag: bits 5-7 FRAMES' when
                                ; drawn, bit 0 it is up (status.inc SPTICK)

; ---- sound ------------------------------------------------------------------
        VAR     SNDF,1          ; frames left of the tone playing
        VAR     SNDND,1         ; the second tone
        VAR     SNDNF,1
        VAR     SNDV,1          ; its volume (VOLAB)
        VAR     MUTED,1

; ---- text and messages --------------------------------------------------------
        VAR     TXCOL,1         ; the XPAND pair text draws in
        VAR     TXSTR,1         ; nonzero: stripe text, the 2nd XPAND pair
        VAR     LINBUF,8        ; number formatting
        VAR     NUMTMP,4        ; NUM32R
        VAR     SHTXK,1         ; XSHTX
        VAR     SHTXR,1
        VAR     SHTXC,1
        VAR     SDMODE,1        ; SDRAWM: text/big, centred, padded
        VAR     SDPAD,1
        VAR     MSGROW,1        ; the message's first row
        VAR     MSGCOL,1
        VAR     MSGSCR,1        ; 1: scrolling along the second row
        VAR     MSGPG,1         ; the string: page, address
        VAR     MSGADR,2
        VAR     MSGPFX,2        ; a RAM string printed first (a name), or 0
        VAR     MSGV1,2         ; the numbers for its '%'s
        VAR     MSGV2,2
        VAR     MSGLEN,1
        VAR     MSGPOS,1
        VAR     MSGLST,2
        VAR     MSGCUT,1
        VAR     MSGK,1          ; walker: characters emitted
        VAR     MSGVN,1         ; walker: which number next
        VAR     MSGSKP,1        ; walker: characters to skip
        VAR     MSGMAX,1        ; walker: characters to emit
        VAR     MSGNP,2         ; walker: the number being emitted

; ---- sprites ------------------------------------------------------------------
SPRN    EQU     10
SPRSZ   EQU     7               ; shape, frame, x lo, x hi, y, col, flags
        VAR     SPRT,SPRN*SPRSZ ; y = 255: not on screen

; ---- the map (the map page) ------------------------------------------------------
        VAR     MAPSRC,1        ; 1: the state is in the window (ENTER/STATE)
        VAR     MAPDOTS,1
        VAR     MAPBLK,1        ; the plot drawn outlined ...
        VAR     MAPBLON,1       ; ... when this is 1
        VAR     BARX,1          ; the time bar (Atari units)
        VAR     BARTOP,1
        VAR     BARH,1
        VAR     BARCOL,1
        VAR     BARLAST,1
        VAR     MPP,1           ; XMAPPLOT's scratch
        VAR     MPO,1
        VAR     MPG,1
        VAR     MPBL,1
        VAR     MPQ,1
        VAR     BDY0,1
        VAR     BDY1,1
        VAR     BDY2,1

; ---- the phase's own (keep this last) --------------------------------------------
        VAR     PHRAM,0
PV      MACRO   nm, sz          ; a phase variable (PVP DEFL PHRAM first)
nm      EQU     PVP
PVP     DEFL    PVP+sz
        ENDM
PVP     DEFL    PHRAM

RAMEND  DEFL    RAMP            ; raised by the phase pages' PV blocks

; ---- the actors (fx.c): at the phase RAM's start, for the phases that use
; them (grant, production, summary); their own variables follow FXEND.
FXN     EQU     4
FXSZ    EQU     24
FXA     EQU     PHRAM
FXEND   EQU     PHRAM+FXN*FXSZ  ; (FXON and FXFLAG are in main RAM: FXSTOP
                                ; touches the actors only if FXON says so)
