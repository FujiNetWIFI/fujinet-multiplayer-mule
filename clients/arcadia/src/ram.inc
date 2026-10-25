; ram.inc -- every byte of RAM the game keeps, and where.
;
; Console RAM, 84 bytes in 26-line mode: zone A $18D0-$18EF, the island
; $18F8-$18FB, zone B $1AD0-$1AFF. Page-0 code reads and writes it
; directly, page-1 code through pointer words. What changes every frame
; lives here.
;
; Cart RAM, 1K (protocol 2), read like ROM and written only through the
; mailbox (CPUT, RAMPUT): RAM-L $0600-$07FF holds the copy of the state
; (build/proto.inc places it, H_ MP_ ST_ G_ A_ TN_ C_) and, past CR_LEND,
; what changes about once a state; RAM-H $2900-$2AFF the wampus list, the
; names and the endpoint, and the phases' own bookkeeping. A write costs
; about 100 clocks; a read from page-0 code 12 (RAM-L) or 18 (RAM-H,
; through a pointer).
;
; Each zone is handed out in order by VAR; AS stops the build when one
; runs over. A phase's own block (UNION) starts over at the same address
; for every phase, so the phases share it.

VAR     MACRO   name,n
name    EQU     RAMNEXT
RAMNEXT SET     RAMNEXT+(n)
        IF      RAMNEXT > RAMEND
        ERROR   "RAM zone full at name"
        ENDIF
        ENDM

ZONE    MACRO   lo,hi
RAMNEXT SET     lo
RAMEND  SET     hi
        ENDM

; ---- zone A: time, input, sound, the display's registers ------------------
        ZONE    $18D0,$18F0
        VAR     TICKH,1         ; the cart's tick at the last frame
        VAR     TICKL,1
        VAR     FRMH,1          ; our frame clock (jiffies), big-endian
        VAR     FRML,1
        VAR     DT,1            ; frames since the loop last came round (1-8)
        VAR     CURPG,1         ; the page in the window
        VAR     AXSEL,1         ; the axis the pot reads next vblank (0 vertical)
        VAR     AXV,1           ; the disc, both axes as last read
        VAR     AXH,1
        VAR     INDIR,1         ; directions held (D_*)
        VAR     INBTN,1         ; FIRE held
        VAR     EVBTN,1         ; FIRE went down this frame
        VAR     EVKEY,1         ; a key went down this frame (K_*, digits)
        VAR     KRAW,1          ; last frame's key (edges)
        VAR     SNDID,1         ; the sound playing ($FF none)
        VAR     SNDCNT,1        ; its frames left
        VAR     SNDQ,1          ; a sound queued ($FF none)
        VAR     DROWH,1         ; the display row's base (DAT, DPOS)
        VAR     DROWL,1
        VAR     DCOLOR,1        ; ink bits for DPUTC
        VAR     DSTRH,1         ; the string DTXT reads
        VAR     DSTRL,1
        VAR     NUMH,1          ; a number for DNUM (24 bits)
        VAR     NUMM,1
        VAR     NUML,1
        VAR     T0,1            ; scratch: never across a call that says so
        VAR     T1,1
        VAR     T2,1
        VAR     T3,1
        VAR     NSTATE,1        ; the network's step (fixb.inc)
        VAR     NWAIT,1         ; frames in it

; ---- the island ($18F0-$18F7 are the sprite registers) ---------------------
        ZONE    $18F8,$18FC
        VAR     ACT,1           ; an action to send ($00 none)
        VAR     ACTARGH,1       ; its argument
        VAR     ACTARGL,1
        VAR     POLLW,1         ; frames to the next poll

; ---- zone B ------------------------------------------------------------------
        ZONE    $1AD0,$1B00
        VAR     SC01SH,1        ; the sprite colour registers (write-only)
        VAR     SC23SH,1
        VAR     BGCB,1          ; BGCOL without the pot mux
        VAR     SHAKE,1         ; VOLUME's top bits: the screen's shift
        VAR     UDCQN,1         ; user-glyph rewrites waiting for the vblank (<= 3)
        VAR     UDCQ,12         ; each: glyph slot, page, source hi, lo
        VAR     ACTSENT,1       ; the action the last reply answers
        VAR     NHAVE,1         ; NH_*: what the last request gave
        VAR     NFAIL,1         ; requests failed in a row
        VAR     INGRUN,1        ; ingest: the run (x5), its byte, the slice shown
        VAR     INGPOS,1
        VAR     INGSLC,1
        VAR     BWH,1           ; STATUS: bytes waiting, last reading
        VAR     BWL,1
        VAR     NSUB,1          ; the network: a count within a step
        VAR     NSEQ,1          ; the SEQ ACKSEQ echoes when it is done
        VAR     PGSAVE,1        ; PGFAR: the page to give back
        VAR     SAYROW,1        ; SAY: the row it is on, rows left, the
        VAR     SAYLFT,1        ;  numbers' digits (v1 << 4 | v2)
        VAR     SAYD,1
        VAR     BARTK,1         ; the time bar's tick (fixb.inc BARTICK)
        VAR     BARTG,1
        VAR     BARFR,1
        VAR     MAPK,1          ; the map: the next plot to look at, the river's phase
        VAR     MAPRV,1
UNIONB  EQU     RAMNEXT         ; the rest: scratch for whoever runs (one frame at most)
UNIONBE EQU     $1B00

; ---- RAM-L past the state copy: about once a state ---------------------------
        ZONE    CR_LEND,$0800
        VAR     STOK,1          ; 1: a state is in and its phase is running
        VAR     LASTPH,1        ; the phase / round last entered ($FF none)
        VAR     LASTRD,1
        VAR     PHPAGE,1        ; the page running the phase
        VAR     SCRNOW,1        ; SCR_*: what the screen shows
        VAR     ANIM,1          ; what the frame handler animates
        VAR     RXPEH,1         ; the phase's elapsed and our frame at the state
        VAR     RXPEL,1
        VAR     RXFRH,1
        VAR     RXFRL,1
        VAR     RXAGE,1         ; how much older the reply is than the fastest
        VAR     CLKBH,1         ; clock_sync's base
        VAR     CLKBL,1
        VAR     CLKHAV,1
        VAR     TESTM,1         ; 1: the offline test mode
        VAR     PGNEW,1         ; 1: PHPAGE changed, its E_ENTER is due
        VAR     DVSTATE,1       ; the development's DV_* (the poll rate needs it)
        VAR     AUTRD,1         ; units traded as shown (pushes carry it)
        VAR     MAPREQ,1        ; 1: the map page draws the whole map (map_enter)
        VAR     MAPON,1         ; 1: the map is up: the loop has it kept current
        VAR     MAPDOTS,1       ; 1: production shown on the plots
        VAR     MAPBLINK,1      ; this plot drawn outlined in white ...
        VAR     MAPBLON,1       ; ... while this is 1
        VAR     MAPHI,1         ; this seat's land outlined in white ($FF none)
        VAR     BARST,1         ; the time bar: steps (0-20), its ink, as drawn
        VAR     BARCOL,1
        VAR     BARDRWN,1
        VAR     FXON,1          ; 1: the sprite scripts run (PG_FX)
        VAR     FXREQ,1         ; a script for the fx page to start ($FF none)
        VAR     FXFLAG,1        ; what a script signals (FX_SIG)
        VAR     MSGROW,1        ; the message box: first row, rows
        VAR     MSGN,1
        VAR     SAYV1H,1        ; SAY's numbers ('%' in its strings)
        VAR     SAYV1L,1
        VAR     SAYV2H,1
        VAR     SAYV2L,1
UNIONL  EQU     RAMNEXT         ; the phase's own, to $07FF
UNIONLE EQU     $0800

; ---- RAM-H: the boot's and the phases' bookkeeping ----------------------------
        ZONE    $2948,$2AF0     ; (the wampus list is $2900-$2947; the staged header $2AF0)
        VAR     SCNAME,9        ; the player's name, NUL-terminated ASCII
        VAR     SCTABLE,9       ; the table id
        VAR     SCENDPT,66      ; the server, from the room appkey (0: the build's)
        VAR     FXA,2*18        ; the fx page's two actors (p_fx.inc AF_*)
        VAR     MAPGL,1         ; the map's peaks still to load (bits: glyphs 4-6)
        VAR     APV,12          ; autoplay's own (p_auto.inc)
        VAR     MUTE,1          ; 1: no sound (Select)
UNIONH  EQU     RAMNEXT
UNIONHE EQU     $2AF0

; screens (mekkogx mule.h)
SCR_NONE EQU    0
SCR_TEXT EQU    1
SCR_MAP  EQU    2
SCR_TOWN EQU    3
SCR_AUC  EQU    4
SCR_SUM  EQU    5
SCR_FOUND EQU   6
AN_NONE  EQU    0
AN_WALK  EQU    1
AN_GRANT EQU    2
AN_AUC   EQU    3
