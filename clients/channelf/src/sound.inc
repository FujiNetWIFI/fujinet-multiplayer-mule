; ---------------------------------------------------------------------------
; sound.inc -- the game's sound effects (sound.h): short blips, no music.
;
; The console has three tones in port 5 bits 7-6 (1: 1 kHz, 2: 500 Hz,
; 3: a buzz) and no volume: a tone rings and decays in about 9 ms, and only
; retriggers when the bits change, so a note goes silence-then-tone. A LOUD
; sound is one retriggered every frame (it sustains); a soft one blips once
; a note. SND_ALERT, the one that says "your hand to the stick", is the only
; two-note sound and the longest. Every port-5 write keeps the VRAM row in
; bits 5-0 (read it back).
;
; Ids from SND_STEP up are soft: they never cut another sound, only each
; other.
; ---------------------------------------------------------------------------

SND_SELECT EQU 0
SND_SAMPLE EQU 1
SND_ERROR  EQU 2
SND_BUY    EQU 3
SND_ALERT  EQU 4
SND_AUOVER EQU 5
SND_TRADE  EQU 6
SND_EVENT  EQU 7
SND_FOUND  EQU 8
SND_STEP   EQU 9
SND_TICK   EQU 10
SND_TOCK   EQU 11

SNDHI   EQU 040H                ; 1 kHz
SNDLO   EQU 080H                ; 500 Hz
SNDBZ   EQU 0C0H                ; the buzz

; {tone, frames, tone 2, frames 2 (0: none), retrigger: 0 once a note,
;  1 every frame, 2 every other frame}
SNDTAB: DB SNDHI,1,  0,0,      0    ; select: a blip
        DB SNDLO,3,  0,0,      1    ; sample
        DB SNDBZ,8,  0,0,      1    ; error
        DB SNDHI,4,  0,0,      1    ; buy
        DB SNDLO,8,  SNDHI,14, 1    ; ALERT
        DB SNDLO,6,  0,0,      1    ; auction over
        DB SNDHI,1,  0,0,      0    ; trade: a blip
        DB SNDBZ,8,  0,0,      2    ; colony event
        DB SNDHI,10, 0,0,      2    ; first founder
        DB SNDBZ,1,  0,0,      0    ; step (soft)
        DB SNDHI,1,  0,0,      0    ; tick (soft)
        DB SNDLO,1,  0,0,      0    ; tock (soft)

SNDAGE  EQU SNDTONE+1           ; frames into the note
SNDMODE EQU SNDTONE+2           ; its retrigger rule

; SNDTRIG -- the tone SNDTONE from silence.  Leaf.  Clobbers A, r8, DC0.
SNDTRIG: INS 5
        NI 03FH
        LR 8,A
        OUTS 5
        LDB SNDTONE
        AS 8
        OUTS 5
        POP

; SNDOFF -- silence.  Leaf.  Clobbers A.
SNDOFF: INS 5
        NI 03FH
        OUTS 5
        POP

; SNDINIT -- nothing playing.  Leaf.  Clobbers A, DC0.
SNDINIT: LI 0FFH
        STB SNDID
        INS 5
        NI 03FH
        OUTS 5
        POP

; SNDPLAY -- play sound r0.  Clobbers A, r0, r8, DC0.
SNDPLAY: ENTER
        LDB SNDMUTE
        NI 0FFH
        BNZ SNDPX
        LR A,0                  ; soft: only over silence or another soft one
        CI SND_STEP-1
        BC SNDP1
        LDB SNDID
        CI SND_STEP-1
        BC SNDPX
SNDP1:  LR A,0
        STB SNDID
        CLR
        STB SNDN
        PI SNDNOTE
SNDPX:  LEAVE

; SNDNOTE -- note SNDN of sound SNDID begins: its tone, length and rule,
;   struck.  Leaf.  Clobbers A, r8, DC0.
SNDNOTE: LDB SNDID
        LR 8,A
        SL 1
        SL 1
        AS 8
        LR 8,A                  ; id * 5
        DCI SNDTAB+4
        ADC
        LM
        STB SNDMODE
        LDB SNDN
        SL 1
        AS 8
        DCI SNDTAB
        ADC
        LM
        LR 8,A                  ; the tone
        LM
        STB SNDT                ; its frames
        LR A,8
        STB SNDTONE
        CLR
        STB SNDAGE
        INS 5                   ; struck: silence, then the tone
        NI 03FH
        LR 8,A
        OUTS 5
        LDB SNDTONE
        AS 8
        OUTS 5
        POP

; SNDTICK -- every frame: notes run out, loud ones are struck again.
;   Clobbers A, r8, DC0.
SNDTICK: ENTER
        LDB SNDID
        CI 0FFH
        BZ SNDTX
        DECB SNDT
        BZ SNDTN
        INCB SNDAGE
        LDB SNDMODE
        CI 1
        BZ SNDTS
        CI 2
        BNZ SNDTX
        LDB SNDAGE
        NI 1
        BNZ SNDTX
SNDTS:  PI SNDTRIG
        BR SNDTX
SNDTN:  LDB SNDN                ; the second note, if there is one
        NI 0FFH
        BNZ SNDTO
        LDB SNDID
        LR 8,A
        SL 1
        SL 1
        AS 8
        DCI SNDTAB+3
        ADC
        LM
        NI 0FFH
        BZ SNDTO
        LIS 1
        STB SNDN
        PI SNDNOTE
        BR SNDTX
SNDTO:  LI 0FFH                 ; over
        STB SNDID
        PI SNDOFF
SNDTX:  LEAVE

; SNDTMUT -- sound off, or on again (TIME).  Leaf.  Clobbers A, DC0.
SNDTMUT: LDB SNDMUTE
        XI 1
        STB SNDMUTE
        LI 0FFH
        STB SNDID
        INS 5
        NI 03FH
        OUTS 5
        POP
