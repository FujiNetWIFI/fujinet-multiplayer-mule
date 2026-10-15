; ---------------------------------------------------------------------------
; mule.asm -- M.U.L.E. (Tournament) for the Fairchild Channel F, over FujiNet.
;
; A hand-written F8 port of the MekkoGX client (clients/mekkogx/src/*.c);
; the comments name the C routine each piece comes from. The server runs
; the game: we poll /state?bin=1 (the 550-byte MuleState of
; clients/mule_proto.h), draw it the Atari's way, and play the animations at
; the Atari's speed.
;
; THE CART. fujinet-firmware pico/channelf at protocol 2: the fixed 16K at
; $0800-$47FF, and one 8K page at a time at $4800-$67FF, chosen by a store to
; $FD80+n (APPBANK); a 60 Hz tick at $FC0E. The image is the 16K and six
; pages, 64K: the fixed block is assembled at 0 PHASEd to $0800, page n at
; $4000 + n * $2000 PHASEd to $4800, so p2bin's 0-$FFFF is the image.
;
; Only SELPG stores to $FD80 (tools/checkcalls.py). Code in a page never
; calls another page; it goes through FAR or resident code.
; ---------------------------------------------------------------------------
        CPU F3850

        INCLUDE "src/equ.inc"
        INCLUDE "src/macros.inc"
        INCLUDE "src/ram.inc"
        INCLUDE "build/flags.inc"
        INCLUDE "src/ids.inc"

; ---- pages --------------------------------------------------------------
PGBOOT  EQU 0                   ; boot, test mode, autoplay
PGSTAT  EQU 1                   ; lobby, summary, game over
PGGRANT EQU 2                   ; land grant, production, colony events
PGAUC   EQU 3                   ; status per good, declare, auctions
PGDEV   EQU 4                   ; development
PGDATA  EQU 5                   ; strings, scripts, art: copied to RAM at boot
NPAGES  EQU 6

; PAGEHDR -- a page's first bytes: its id twice (the second complemented),
; then the phase entries the loop calls.
PAGEHDR MACRO id, e1, e2, e3, e4
        DB id, (~(id))&0FFH
        JMP e1
        JMP e2
        JMP e3
        JMP e4
        ENDM
PE_ENTER EQU PGWIN+2
PE_STATE EQU PGWIN+5
PE_TICK  EQU PGWIN+8
PE_ANIM  EQU PGWIN+11

; =======================================================================
; the fixed 16K: $0800-$47FB
; =======================================================================
        ORG 0
        PHASE 0800H
MB_FIX:
        DB 055H                 ; the BIOS's cart signature
        DB 000H                 ; ($0801 is skipped)
        JMP START               ; $0802

        INCLUDE "src/start.inc"
MB_LOOP:
        INCLUDE "src/loop.inc"
MB_NET:
        INCLUDE "src/net.inc"
MB_INPUT:
        INCLUDE "src/input.inc"
MB_SOUND:
        INCLUDE "src/sound.inc"
MB_TEST:
        INCLUDE "src/test.inc"
MB_STUBS:
UENDPTX:
        INCLUDE "build/endpoint.inc"
MB_DISP:
        INCLUDE "src/disp.inc"
MB_SPR:
        INCLUDE "src/spr.inc"
MB_MAP:
        INCLUDE "src/map.inc"
MB_FX:
        INCLUDE "src/fx.inc"
MB_GAME:
        INCLUDE "src/game.inc"
MB_TEXT:
        INCLUDE "src/text.inc"
MB_MATH:
        INCLUDE "src/math.inc"
MB_FONT:
        INCLUDE "gen/font.inc"
MB_FIXEND:
        IF $ > 047FCH
        ERROR "the fixed block runs into the claim"
        ENDIF
        DEPHASE

; the claim: the cart keeps the mailbox alive for an image that has it
        ORG 03FFCH
        DB "FUJI"

; =======================================================================
; the pages: $4800-$67FF
; =======================================================================
PAGE    MACRO n
        ORG 04000H+(n)*02000H
        PHASE PGWIN
        ENDM

        PAGE PGBOOT
MB_P0:  PAGEHDR PGBOOT, STUB, STUB, STUB, STUB
        INCLUDE "src/p0boot.inc"
MB_P0END:
        DEPHASE

        PAGE PGSTAT
MB_P1:  PAGEHDR PGSTAT, STUB, STUB, STUB, STUB
        INCLUDE "src/p1stat.inc"
MB_P1END:
        DEPHASE

        PAGE PGGRANT
MB_P2:  PAGEHDR PGGRANT, STUB, STUB, STUB, STUB
        INCLUDE "src/p2grant.inc"
MB_P2END:
        DEPHASE

        PAGE PGAUC
MB_P3:  PAGEHDR PGAUC, STUB, STUB, STUB, STUB
        INCLUDE "src/p3auc.inc"
MB_P3END:
        DEPHASE

        PAGE PGDEV
MB_P4:  PAGEHDR PGDEV, STUB, STUB, STUB, STUB
        INCLUDE "src/p4dev.inc"
MB_P4END:
        DEPHASE

; The data page is assembled where START copies it: DATA, in RAM.
        ORG 04000H+PGDATA*02000H
        PHASE DATA
MB_P5:  DB PGDATA, (~PGDATA)&0FFH
        INCLUDE "gen/str.inc"
        INCLUDE "gen/art.inc"
        INCLUDE "src/fxdata.inc"
MB_P5END:
DATALEN EQU $-DATA
        IF DATALEN > PGSIZE
        ERROR "the data page is over 8K"
        ENDIF
        DEPHASE

        ORG 0FFFFH
        DB 0FFH                 ; (p2bin: the image is all 64K)
        END
