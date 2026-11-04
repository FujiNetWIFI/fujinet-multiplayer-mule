; sound.inc -- the effects (sound.h): short blips, no music. Fixed.
;
; The Adam's effects (clients/mekkogx/src/adam/sound.c: the same SN76489 at
; the same clock), played by OS7's sound engine: each effect is a song of
; one or two notes on tone 1, all sharing the one data area (SNDAREA), so a
; new effect takes the place of the one playing; the frame interrupt runs
; PLAY_SONGS and SOUND_MAN. A delay d of the Apple II's table is a divisor
; of 3.07 d. The alert (SNTURN) is the only effect with two notes, and the
; loudest; the step and the time bar's tick are soft and never cut another
; effect.

SNSEL   EQU     0
SNSAMP  EQU     1
SNERR   EQU     2
SNBUY   EQU     3
SNTURN  EQU     4
SNAUCO  EQU     5
SNTRADE EQU     6
SNEVT   EQU     7
SNFOUND EQU     8
SNSTEP  EQU     9               ; soft: SNSTEP and up
SNTICK  EQU     10
SNTOCK  EQU     11
SNQUIET EQU     12              ; (a rest: stops whatever plays)

; NOTE d, frames, attenuation: one fixed note on tone 1 (OS7's format:
; channel and type, divisor low, attenuation and divisor high, frames).
NOTE    MACRO   dly, frm, att
        DB      40H
        DB      (dly*3+(dly*18)/256) AND 0FFH
        DB      att*16+(dly*3+(dly*18)/256)/256
        DB      frm
        ENDM
SNEND   EQU     50H             ; tone 1: the end of the song

SNDS:   DW      SNG0,SNDAREA,SNG1,SNDAREA,SNG2,SNDAREA,SNG3,SNDAREA
        DW      SNG4,SNDAREA,SNG5,SNDAREA,SNG6,SNDAREA,SNG7,SNDAREA
        DW      SNG8,SNDAREA,SNG9,SNDAREA,SNG10,SNDAREA,SNG11,SNDAREA
        DW      SNG12,SNDAREA
SNG0:    NOTE    42,3,4          ; select
        DB      SNEND
SNG1:    NOTE    132,3,4         ; sample
        DB      SNEND
SNG2:    NOTE    255,8,4         ; error (as low as it goes)
        DB      SNEND
SNG3:    NOTE    33,4,4          ; buy
        DB      SNEND
SNG4:    NOTE    35,8,0          ; the alert
        NOTE    25,14,0
        DB      SNEND
SNG5:    NOTE    70,6,4          ; auction over
        DB      SNEND
SNG6:    NOTE    30,2,4          ; trade
        DB      SNEND
SNG7:    NOTE    165,8,4         ; event
        DB      SNEND
SNG8:    NOTE    47,10,4         ; founder
        DB      SNEND
SNG9:    NOTE    200,1,8         ; step: a soft click
        DB      SNEND
SNG10:   NOTE    13,2,8          ; tick (the time bar running out)
        DB      SNEND
SNG11:   NOTE    18,2,8          ; tock
        DB      SNEND
SNG12:   DB      61H             ; a frame's rest on tone 1
        DB      SNEND

; SNDINI: the engine, the chip quiet. (The header skips the BIOS's title
; screen, and with it the chip's silencing.)
SNDINI: LD      B,1
        LD      HL,SNDS
        CALL    SNDINIT
        JP      SNDOFF

; SNDALRT: the alert, for a seated player. Preserves BC,DE,HL.
SNDALRT: LD     A,(MYSEAT)
        CP      4
        RET     NC
        LD      A,SNTURN
        ; fall into SNDPLAY

; SNDPLAY: effect A. Preserves BC,DE,HL,IX,IY.
SNDPLAY: PUSH   HL
        LD      L,A
        LD      A,(MUTED)
        OR      A
        JR      NZ,SNP9
        LD      A,L
        CP      SNSTEP
        JR      C,SNP1
        LD      A,(SNDAREA)     ; a soft one: only over quiet or another
        INC     A               ; soft one
        JR      Z,SNP1
        LD      A,(SNDCUR)
        CP      SNSTEP
        JR      C,SNP9
SNP1:   LD      A,L
        CALL    SNDGO
SNP9:   POP     HL
        RET

; SNDGO: song A + 1 now (the frame interrupt kept out meanwhile).
SNDGO:  LD      (SNDCUR),A
        PUSH    BC
        PUSH    DE
        PUSH    IX
        PUSH    IY
        INC     A
        LD      B,A
        CALL    VLOCK
        CALL    PLAYIT
        CALL    VFREE
        POP     IY
        POP     IX
        POP     DE
        POP     BC
        RET

; SNDTICK: nothing: the frame interrupt keeps the time.
SNDTICK: RET

SNDMUTE: LD     A,(MUTED)
        XOR     1
        LD      (MUTED),A
        PUSH    HL
        LD      A,SNQUIET
        CALL    SNDGO
        POP     HL
        RET
