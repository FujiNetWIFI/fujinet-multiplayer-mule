; mule.asm -- M.U.L.E. (Tournament) for the Bally Astrocade, over FujiNet.
;
; A hand-assembled port of the MekkoGX client (clients/mekkogx/src/*.c,
; itself a port of clients/intv); the comments name the C routine each piece
; comes from. The server runs the game; we poll /state?bin=1 (the 550-byte
; MuleState of clients/mule_proto.h), draw it the Atari's way and play the
; animations at the Atari's speed.
;
; A BANKED cartridge (firmware protocol v2, APPBANK), laid out like gcal's:
; the image is the 8K window plus 4K pages; one read at FNBKSEL+page maps a
; page into 2000H-2FFFH. 3000H-3AFFH never moves: the resident core (the
; frame interrupt, the mailbox, the network engine, the frame loop, input,
; sound, text, the message line) lives there. Each page opens with the 55H
; header whose start vector is RSTENT, so a console RESET with any page
; mapped comes back to a cold start, then the phase entries (loop.inc).
; Only SELPG switches pages, and code in a page never switches away from
; itself: it goes through FARCALL or resident code.
;
; Interrupts: IM 2 with I = 4FH. The family keeps interrupts off so that I
; stays 0 and refresh cycles never land on the cart's hotspot pages; I = 4FH
; points them at screen RAM instead (fuji_mailbox.h names 4FH as the safe
; value), the vector word sits in RAM, and the handler (resident) reads no
; cart hotspot. The screen's line interrupt below the picture counts frames.

        INCLUDE "HVGLIB.H"
        INCLUDE "fujinet.inc"
        INCLUDE "ram.inc"
        INCLUDE "build/flags.inc"

; ---- pages ------------------------------------------------------------
PGBOOT  EQU     0               ; boot: title, name, tables, appkeys
PGSTAT  EQU     2               ; lobby, summary, game over
PGGRANT EQU     3               ; map, land grant, production, events, fx
PGAUC   EQU     4               ; status per good, declare, auctions
PGDEV   EQU     5               ; development
PGDEV2  EQU     6               ; development, continued
PGSTRA  EQU     7               ; strings: titles, messages, words ...
PGSTRB  EQU     8               ; strings: player events, ratings
PGTEST  EQU     9               ; offline test mode, autoplay
PGNET   EQU     10              ; the request engine, appkeys, a state's arrival
PGSPR   EQU     11              ; the sprites: blitter and shapes
PGMAP   EQU     12              ; the map's plots and logic, the town, fills
NPAGES  EQU     13

INLINV  EQU     LINES*2         ; the line interrupt: just below the picture
INMODV  EQU     08H             ; line interrupt on, held until acknowledged

; COLSET order: left palette 3..0, then right 3..0. One palette for the
; whole screen (HORCB 0): white, orange, blue, black -- from MAME's
; astrocade_palette maths, the nearest to the Atari's $0E, $34, $84.
PALW    EQU     07H
PALO    EQU     6BH
PALB    EQU     0F3H
PALK    EQU     00H

; PAGEHDR: every page's first bytes: the header, then the phase entries.
PAGEHDR MACRO   pe1, pe2, pe3, pe4
        DB      55H
        DW      MENUST
        DW      RNAME
        DW      RSTENT
        JP      pe1
        JP      pe2
        JP      pe3
        JP      pe4
        ENDM

; =======================================================================
; page 0: boot
; =======================================================================
        ORG     FIRSTC
MB_P0:
        PAGEHDR STUBRET,STUBRET,STUBRET,STUBRET

PRGSTR: DI
        LD      SP,STACK
        SYSTEM  INTPC
        DO      SETOUT
        DB      LINES*2
        DB      0               ; HORCB: one palette, black border
        DB      0               ; INMOD: nothing yet
        DO      COLSET
        DW      PALET
        EXIT
        ; the whole of RAM below the stack: screen and variables (SELECT
        ; GAME leaves text in the variable area -- news found that)
        LD      HL,NORMEM
        LD      DE,NORMEM+1
        LD      BC,STKLOW-NORMEM-1
        LD      (HL),0
        LDIR
        LD      A,PGBOOT
        LD      (CURPG),A
        LD      A,CW
        LD      (MSGCOL),A
        LD      A,11
        LD      (MSGROW),A
        CALL    INTON
        JP      BOOT

MB_P0_BOOT:
        INCLUDE "boot.inc"
MB_P0END:

; =======================================================================
; resident: 3000H-3AFFH
; =======================================================================
        ORG     3000H
MB_R:
RSTENT: LD      A,(FNBKSEL+0)   ; RESET with any page mapped: page 0, cold
        JP      PRGSTR
RNAME:  DB      "M.U.L.E.",0

MB_R_CORE:
        INCLUDE "core.inc"
MB_R_LOOP:
        INCLUDE "loop.inc"
MB_R_WIN:
        INCLUDE "win.inc"
MB_R_MSG:
        INCLUDE "msg.inc"
MB_R_INPUT:
        INCLUDE "input.inc"
MB_R_SOUND:
        INCLUDE "sound.inc"
MB_R_GFX:
        INCLUDE "gfx.inc"
        INCLUDE "gfx2.inc"
MB_R_TEXT:
        INCLUDE "text.inc"
MB_R_FONT:
        INCLUDE "font46.inc"
MB_R_FUJI:
        INCLUDE "fujilib.inc"
MB_R_DATA:
PALET:  DB      PALW,PALO,PALB,PALK
        DB      PALW,PALO,PALB,PALK
        INCLUDE "stubs.inc"
        INCLUDE "build/endpoint.inc"
MB_REND:

; =======================================================================
; page 2: status (lobby, summary, game over)
; =======================================================================
        ORG     2000H+PGSTAT*1000H
        PHASE   2000H
MB_P2:
        PAGEHDR STENTER,STSTATE,STTICK,STANIM
MB_P2_STATUS:
        INCLUDE "status.inc"
MB_P2END:
        DEPHASE

; =======================================================================
; page 3: map, grant, production (M2)
; =======================================================================
        ORG     2000H+PGGRANT*1000H
        PHASE   2000H
MB_P3:
        PAGEHDR G3ENTER,G3STATE,G3TICK,G3ANIM
MB_P3_FX:
        INCLUDE "fx.inc"
MB_P3_GRANT:
        INCLUDE "grant.inc"
MB_P3END:
        DEPHASE

; =======================================================================
; page 4: auctions (M4)
; =======================================================================
        ORG     2000H+PGAUC*1000H
        PHASE   2000H
MB_P4:
        PAGEHDR A4ENTER,AUSTATE,AUTICK,A4ANIM
MB_P4_AUC:
        INCLUDE "auction.inc"
MB_P4END:
        DEPHASE

; =======================================================================
; pages 5, 6: development (M3)
; =======================================================================
        ORG     2000H+PGDEV*1000H
        PHASE   2000H
MB_P5:
        PAGEHDR D5ENTER,DVSTATEF,DVTICK,D5ANIM
MB_P5_DEV:
        INCLUDE "dev1.inc"
MB_P5END:
        DEPHASE
        ORG     2000H+PGDEV2*1000H
        PHASE   2000H
MB_P6:
        PAGEHDR STUBRET,STUBRET,STUBRET,STUBRET
MB_P6_DEV:
        INCLUDE "dev2.inc"
        INCLUDE "dev2b.inc"
MB_P6END:
        DEPHASE

; =======================================================================
; pages 7, 8: the strings
; =======================================================================
        ORG     2000H+PGSTRA*1000H
        PHASE   2000H
MB_P7:
        PAGEHDR STUBRET,STUBRET,STUBRET,STUBRET
MB_P7_MSG:
        INCLUDE "msgpg.inc"
        INCLUDE "msgpg2.inc"
MB_P7_STR:
        INCLUDE "gen/stra.inc"
MB_P7END:
        DEPHASE
        ORG     2000H+PGSTRB*1000H
        PHASE   2000H
MB_P8:
        PAGEHDR STUBRET,STUBRET,STUBRET,STUBRET
        INCLUDE "gen/strb.inc"
MB_P8END:
        DEPHASE

; =======================================================================
; page 9: the offline test mode, autoplay
; =======================================================================
        ORG     2000H+PGTEST*1000H
        PHASE   2000H
MB_P9:
        PAGEHDR STUBRET,STUBRET,STUBRET,STUBRET
        INCLUDE "test.inc"
MB_P9END:
        DEPHASE

; =======================================================================
; page 10: the network
; =======================================================================
        ORG     2000H+PGNET*1000H
        PHASE   2000H
MB_P10:
        PAGEHDR STUBRET,STUBRET,STUBRET,STUBRET
        INCLUDE "netpg.inc"
MB_P10END:
        DEPHASE

; =======================================================================
; page 11: the sprites
; =======================================================================
        ORG     2000H+PGSPR*1000H
        PHASE   2000H
MB_P11:
        PAGEHDR STUBRET,STUBRET,STUBRET,STUBRET
MB_P11_CODE:
        INCLUDE "sprpg.inc"
MB_P11_ART:
        INCLUDE "gen/spr.inc"
MB_P11END:
        DEPHASE

; =======================================================================
; page 12: the map, the town
; =======================================================================
        ORG     2000H+PGMAP*1000H
        PHASE   2000H
MB_P12:
        PAGEHDR STUBRET,STUBRET,STUBRET,STUBRET
MB_P12_CODE:
        INCLUDE "mappg.inc"
MB_P12_ART:
        INCLUDE "gen/map.inc"
MB_P12END:
        DEPHASE
