; sound.inc -- the effects on the music processor (sound.h). Resident.
;
; The table is the Apple II's (clients/mekkogx/src/adam/sound.c): a delay d
; there is a tone of 36428/d Hz, held so many frames, then maybe a second.
; Here tone A with the master oscillator at 32: f = 1789773 / (33 (T+1) 2),
; so T+1 = 0.744 d (computed below, in the table).

SNSEL   EQU     0
SNSAMP  EQU     1
SNERR   EQU     2
SNBUY   EQU     3
SNTURN  EQU     4
SNDECL  EQU     5
SNAUCO  EQU     6
SNTRADE EQU     7
SNTICK  EQU     8
SNEVT   EQU     9
SNFOUND EQU     10
SNBOOT  EQU     11
SNSTEP  EQU     12

TONEA   EQU     11H
SNDMO   EQU     32

TN      MACRO   dly
        DB      (dly*744+500)/1000-1
        ENDM

; {tone, frames, tone2, frames2}
SNDTAB: TN      42
        DB      4,0,0           ; select
        TN      132
        DB      6,0,0           ; sample
        TN      255
        DB      12,0,0          ; error
        TN      33
        DB      5
        TN      25
        DB      8               ; buy
        TN      56
        DB      10
        TN      42
        DB      14              ; my turn
        TN      70
        DB      8
        TN      53
        DB      8               ; declare
        TN      53
        DB      10
        TN      70
        DB      16              ; auction over
        TN      30
        DB      3,0,0           ; trade
        TN      83
        DB      2,0,0           ; tick
        TN      165
        DB      20
        TN      231
        DB      20              ; event
        TN      70
        DB      20
        TN      35
        DB      40              ; founder
        TN      106
        DB      10
        TN      70
        DB      20              ; boot
        TN      200
        DB      1,0,0           ; step

; SNDPLAY: effect A. Preserves BC,DE (and HL).
SNDPLAY: PUSH   HL
        LD      L,A
        LD      A,(MUTED)
        OR      A
        JR      NZ,SNP9
        LD      A,L
        CP      SNSTEP
        JR      NZ,SNP1
        LD      A,(SNDF)        ; a footstep never cuts another sound
        OR      A
        JR      NZ,SNP9
        LD      A,L
SNP1:   ADD     A,A
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

; SNDTONE: tone period A on tone A, loud (the step's soft).
SNDTONE: PUSH   AF
        LD      A,SNDMO
        OUT     (TONMO),A
        POP     AF
        OUT     (TONEA),A
        CP      (200*744+500)/1000-1
        LD      A,0AH
        JR      NZ,SNT1
        LD      A,03H
SNT1:   OUT     (VOLAB),A
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
