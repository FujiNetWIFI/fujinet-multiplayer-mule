; ram.inc -- console RAM ($0800-$09FF), the ledger. $0800-$080F are the
; runtime's (s2macro.inc); page $08 is reached through RA (VAR/GETV/PUTV);
; page $09 holds the text buffer and, from the top down, the stack.

TESTHK  EQU     V_APP+00H       ; 4: the harness's hook (main: 3C C3, 5A ..., A5)
FCL     EQU     V_APP+04H       ; our frame clock, RE + FADJ (mekkogx frame_count)
FCH     EQU     V_APP+05H
FADJL   EQU     V_APP+06H       ; frames the clock was put on (clock sync)
FADJH   EQU     V_APP+07H
FWLAST  EQU     V_APP+08H       ; RE.0 at the last frame_wait
DONEL   EQU     V_APP+09H       ; the frame the self-moving things are at
DONEH   EQU     V_APP+0AH
CURBANK EQU     V_APP+0BH       ; the bank in the window
KEY     EQU     V_APP+0CH       ; the key held (0-9, K_NONE)
KEDGE   EQU     V_APP+0DH       ; a key pressed this frame (or repeating)
KLAST   EQU     V_APP+0EH       ; KEY the frame before
KREPT   EQU     V_APP+0FH       ; FCL when a held key repeats next
KREPON  EQU     V_APP+10H       ; held keys repeat (menus, the name)
MUTE    EQU     V_APP+11H
SNDPH   EQU     V_APP+12H       ; the cue's next (on, off) pair; 0 none
SNDPL   EQU     V_APP+13H
SNDT    EQU     V_APP+14H       ; FCL when that pair starts
SNDID   EQU     V_APP+15H       ; the cue playing
; the network (net.inc)
NSTATE  EQU     V_APP+16H
NWAIT   EQU     V_APP+17H       ; frames without an answer
NSUB    EQU     V_APP+18H
NSEQ    EQU     V_APP+19H       ; the SEQ posted (ACKSEQ echoes it)
BWL     EQU     V_APP+1AH       ; STATUS: bytes waiting, last reading
BWH     EQU     V_APP+1BH
ACT     EQU     V_APP+1CH       ; the queued action
ACTARGL EQU     V_APP+1DH
ACTARGH EQU     V_APP+1EH
ACTSENT EQU     V_APP+1FH
NHAVE   EQU     V_APP+20H
NFAIL   EQU     V_APP+21H
POLLW   EQU     V_APP+22H       ; frames to the next poll
AUTRADE EQU     V_APP+23H       ; units traded as shown (with pushes)
CPDONE  EQU     V_APP+24H       ; FN_COPYDONE before our COPY
; the state (mekkogx main.c)
STOK    EQU     V_APP+25H
PHASE   EQU     V_APP+26H
RND     EQU     V_APP+27H
MYSEAT  EQU     V_APP+28H
LASTPH  EQU     V_APP+29H
LASTRD  EQU     V_APP+2AH
RXPEL   EQU     V_APP+2BH       ; phase-elapsed at the last state
RXPEH   EQU     V_APP+2CH
RXFRL   EQU     V_APP+2DH       ; our frame then (less RXAGE)
RXFRH   EQU     V_APP+2EH
RXAGE   EQU     V_APP+2FH
CLKHAV  EQU     V_APP+30H
CLKBL   EQU     V_APP+31H
CLKBH   EQU     V_APP+32H
TESTM   EQU     V_APP+33H
ANIM    EQU     V_APP+34H
SCRNOW  EQU     V_APP+35H
DVSTATE EQU     V_APP+36H
; text (text.inc)
MSGROW  EQU     V_APP+37H
MSGLEN  EQU     V_APP+38H
MSGPOS  EQU     V_APP+39H
MSGSCR  EQU     V_APP+3AH
MSGT    EQU     V_APP+3BH
V1L     EQU     V_APP+3CH       ; the numbers a string's '%'s take
V1H     EQU     V_APP+3DH
V2L     EQU     V_APP+3EH
V2H     EQU     V_APP+3FH
TXTLEN  EQU     V_APP+46H
T0      EQU     V_APP+47H       ; scratch for one routine at a time
T1      EQU     V_APP+48H
T2      EQU     V_APP+49H
T3      EQU     V_APP+4AH
T4      EQU     V_APP+4BH
T5      EQU     V_APP+4CH
SPECAT  EQU     V_APP+4DH
CUN     EQU     V_APP+4EH       ; catchup's count
CPLEN   EQU     V_APP+4FH       ; the COPY under way: 0 the state, else a header
; the map (map.inc)
MAPQ    EQU     V_APP+50H       ; the next plot of a whole redraw (45: done)
MAPDOTS EQU     V_APP+51H
MAPBLINK EQU    V_APP+52H
MAPBLON EQU     V_APP+53H
MPP     EQU     V_APP+54H       ; the plot being drawn: number, corner,
MPX     EQU     V_APP+55H       ; terrain, owner, M.U.L.E.
MPY     EQU     V_APP+56H
MPT     EQU     V_APP+57H
MPO     EQU     V_APP+58H
MPG     EQU     V_APP+59H
MSVN    EQU     V_APP+5AH
MSVP    EQU     V_APP+5BH
BARLAST EQU     V_APP+5CH
BARTK   EQU     V_APP+5DH
BARTG   EQU     V_APP+5EH
BARFR   EQU     V_APP+5FH
MAPDIRTY EQU    V_APP+60H       ; 6: a plot to draw again, a bit each
BARO    EQU     V_APP+40H       ; the time bar: 0 down the map, 1 across the town
BARMAX  EQU     V_APP+41H       ; its length
BARTOPV EQU     V_APP+42H       ; down an edge: its top line
MSG1    EQU     V_APP+43H       ; the message has one row (MSGROW), not two
SEED    EQU     V_APP+44H       ; rand8's
; the actors (fx.inc): an array of 4 a field
FXST    EQU     V_APP+66H
FXX     EQU     FXST+4
FXY     EQU     FXST+8
FXWAIT  EQU     FXST+12
FXN     EQU     FXST+16
FXPER   EQU     FXST+20
FXDX    EQU     FXST+24
FXDY    EQU     FXST+28
FXKIND  EQU     FXST+32
FXSLOT  EQU     FXST+36
FXIMG   EQU     FXST+40
FXFRAME EQU     FXST+44
FXD2    EQU     FXST+48
FXREP   EQU     FXST+52
FXMODE  EQU     FXST+56
FXPCH   EQU     FXST+60
FXPCL   EQU     FXST+64
FXON    EQU     FXST+68
FXFLAG  EQU     FXST+69
FXI     EQU     FXST+70
; ... phases: PH (a union) to the end of the page
PH      EQU     FXST+71         ; each phase's own (see its file)
PHEND   EQU     V_APP+0F0H

        IF      HI(PHEND-1) <> HI(RAMVARS)
        ERROR   "RAM ledger runs off page $08"
        ENDIF

TXT     EQU     0900H           ; 144: the text being shown (str_get)
TXTMAX  EQU     140
NUMBUF  EQU     0990H           ; 12: fmt_num's and fmt32's digits, NUL-ended
NUM32   EQU     099CH           ; 4: fmt32's number
PH9     EQU     09A0H           ; 32: more of a phase's own
STKLOW  EQU     09C0H           ; the stack grows down from $09FF to here
