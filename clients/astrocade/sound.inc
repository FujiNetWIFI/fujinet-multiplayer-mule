; sound.inc -- the effects on the music processor (sound.h): short blips, no
; music. Resident.
;
; The table is the Apple II's (clients/mekkogx/src/apple2/sound.c): a delay
; d there is a tone of 36428/d Hz, held so many frames, then maybe a second.
; Here tone A with the master oscillator at 32: f = 1789773 / (33 (T+1) 2),
; so T+1 = 0.744 d (computed below, in the table). SNTURN is the one that
; matters: rung whenever the game wants your hand on the stick (SNDALRT),
; it is the only effect with two notes, and the loudest.

SNSEL   EQU     0
SNSAMP  EQU     1
SNERR   EQU     2
SNBUY   EQU     3
SNTURN  EQU     4
SNAUCO  EQU     5
SNTRADE EQU     6
SNEVT   EQU     7
SNFOUND EQU     8
SNSTEP  EQU     9

TONEA   EQU     11H
SNDMO   EQU     32

TN      MACRO   dly
        DB      (dly*744+500)/1000-1
        ENDM

; {tone, frames, tone2, frames2}
SNDTAB: TN      42
        DB      3,0,0           ; select
        TN      132
        DB      3,0,0           ; sample
        TN      255
        DB      8,0,0           ; error
        TN      33
        DB      4,0,0           ; buy
        TN      35
        DB      8
        TN      25
        DB      14              ; the alert
        TN      70
        DB      6,0,0           ; auction over
        TN      30
        DB      2,0,0           ; trade
        TN      165
        DB      8,0,0           ; event
        TN      47
        DB      10,0,0          ; founder
        TN      200
        DB      1,0,0           ; step

; SNDALRT: the alert, for a seated player. Preserves BC,DE,HL.
SNDALRT: LD     A,(MYSEAT)
        CP      4
        RET     NC
        LD      A,SNTURN
        ; fall into SNDPLAY

; SNDPLAY: effect A. Preserves BC,DE (and HL).
SNDPLAY: PUSH   HL
        LD      L,A
        LD      A,(MUTED)
        OR      A
        JR      NZ,SNP9
        LD      A,L
        LD      H,03H           ; the step soft
        CP      SNSTEP
        JR      NZ,SNP0
        LD      A,(SNDF)        ; a footstep never cuts another sound
        OR      A
        JR      NZ,SNP9
        JR      SNP1
SNP0:   LD      H,0FH           ; the alert loud
        CP      SNTURN
        JR      Z,SNP1
        LD      H,07H
SNP1:   LD      A,H
        LD      (SNDV),A
        LD      A,L
        ADD     A,A
        ADD     A,A
        LD      L,A
        LD      H,0
        PUSH    DE
        LD      DE,SNDTAB
        ADD     HL,DE
        POP     DE
        LD      A,(HL)
        INC     HL
        CALL    SNDTONE
        LD      A,(HL)
        INC     HL
        LD      (SNDF),A
        LD      A,(HL)
        INC     HL
        LD      (SNDND),A
        LD      A,(HL)
        LD      (SNDNF),A
SNP9:   POP     HL
        RET

; SNDTONE: tone period A on tone A, at the effect's volume.
SNDTONE: PUSH   AF
        LD      A,SNDMO
        OUT     (TONMO),A
        POP     AF
        OUT     (TONEA),A
        LD      A,(SNDV)
        OUT     (VOLAB),A
        RET

SNDTICK: LD     A,(SNDF)
        OR      A
        RET     Z
        DEC     A
        LD      (SNDF),A
        RET     NZ
        LD      A,(SNDNF)
        OR      A
        JR      Z,SNDOFF
        LD      (SNDF),A
        XOR     A
        LD      (SNDNF),A
        LD      A,(SNDND)
        JR      SNDTONE
SNDOFF: XOR     A
        OUT     (VOLAB),A
        RET

SNDMUTE: LD     A,(MUTED)
        XOR     1
        LD      (MUTED),A
        XOR     A
        LD      (SNDF),A
        JR      SNDOFF
