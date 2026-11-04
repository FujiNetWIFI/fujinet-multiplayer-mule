; os7.inc -- the ColecoVision BIOS (OS7) the client uses, and the hardware.
;
; Entry points and register conventions from ~/Workspace/os7lib (src/os7.h);
; the sound engine's RAM and note format checked against the BIOS itself.
; The traps fujinet-fujitzee/coleco/README.md found apply here too:
;   - WRITE_REGISTER takes the register in B and the value in C;
;   - WRITE_VRAM mishandles a count past 256 that isn't a whole page;
;   - RAND_GEN's seed has a fixed point at 0: seed it;
;   - a cartridge whose header starts 55H AAH skips the title screen and
;     with it the sound chip's silencing: the client silences it itself.

; ---- the BIOS entries --------------------------------------------------
WRITREG EQU     1FD9H           ; B = register, C = value
READREG EQU     1FDCH           ; A = the VDP status (acknowledges the frame)
POLLER  EQU     1FEBH           ; scan the controllers into the header's area
SNDINIT EQU     1FEEH           ; B = sound data areas, HL = the song table
PLAYIT  EQU     1FF1H           ; B = song number (1..)
SNDMAN  EQU     1FF4H           ; once a frame, after PLAYSNG
PLAYSNG EQU     1F61H           ; once a frame: the notes out to the chip
SNDOFF  EQU     1FD6H           ; every channel quiet
RANDGEN EQU     1FFDH           ; A = a random byte (and RANDNUM stirred)

; ---- the BIOS's own RAM ---------------------------------------------------
OS7SND  EQU     7020H           ; 7020H-702AH: the sound engine's pointers
DEFERWR EQU     73C6H           ; nonzero: VRAM writes queue for WRITER
MUXSPRT EQU     73C7H
RANDNUM EQU     73C8H           ; RAND_GEN's seed, 16 bits
OS7STK  EQU     73B9H           ; where the BIOS starts the stack; 73B9H up
                                ; is the BIOS's

; ---- the controller area the header points at (POLLER fills it) ----------
; 0, 1: enables; then for player 1: 2 left fire, 3 stick, 4 spinner,
; 5 right fire, 6 keypad; then player 2 the same.
CT_EN1  EQU     0
CT_EN2  EQU     1
CT_LFIRE EQU    2               ; bit 6: pressed
CT_STICK EQU    3               ; 1 up, 2 right, 4 down, 8 left
CT_RFIRE EQU    5
CT_KEY  EQU     6               ; 0-9, 0AH '*', 0BH '#', 0FH none

; ---- the chips ----------------------------------------------------------
VDPD    EQU     0BEH            ; TMS9918A data
VDPC    EQU     0BFH            ; TMS9918A control
SNPORT  EQU     0FFH            ; SN76489 (writes)

; ---- TMS9918A colours -------------------------------------------------------
T_TRANS EQU     0
T_BLACK EQU     1
T_MGRN  EQU     2
T_LGRN  EQU     3
T_DBLUE EQU     4
T_LBLUE EQU     5
T_DRED  EQU     6
T_CYAN  EQU     7
T_MRED  EQU     8
T_LRED  EQU     9
T_DYEL  EQU     10
T_LYEL  EQU     11
T_DGRN  EQU     12
T_MAG   EQU     13
T_GREY  EQU     14
T_WHITE EQU     15
T_PLAINS EQU    T_GREY          ; the map's ground
T_RIVER EQU     T_DBLUE

; ---- VRAM (Graphics II with a sequential name table: a bitmap) ------------
VR_PAT  EQU     0000H           ; cell row * 256 + cell * 8 + line
VR_NAME EQU     1800H
VR_SAT  EQU     1B00H           ; sprite attributes
VR_SPARE EQU    1B80H           ; 1B80H-1FFFH: ours (see ram.inc)
VR_COL  EQU     2000H           ; colours, laid out as the patterns
VR_SPAT EQU     3800H           ; sprite patterns, 32 bytes each (16 x 16)
