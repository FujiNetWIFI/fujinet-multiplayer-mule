; snd.inc -- the sounds (mekkogx sound.h), played by the main loop: a page
; queues one with beep (fixed.inc), snd_go starts it, the vblank counts it
; down and stops it.
;
; The 8244's sound is a 24-bit pattern rotating out of $A7-$A9, one bit a
; shift: every 4 lines with control bit 5 (about 3933 a second), else every
; 16 (983). A pattern repeating every 2, 3, 4, 6, 8, 12 or 24 bits makes a
; tone of that rate over the period: 1966, 1311, 983, 655, 491, 328, 164 Hz
; on the fast clock. Control: bit 7 on, 5 the fast clock, 4 noise, 3-0 the
; volume (MAME i8244.cpp; bit 6 is nothing). Each mekkogx sound takes the
; tone nearest the Atari's (36428 / AUDF Hz).

; snd_go: the sound queued, if one is: its pattern, its length into SNDCNT,
; then on. Clobbers A, r0, r1, r5, r6.
snd_go:
    mov r1,#SNDCNT
    mov a,@r1
    jb7 sg_play
    ret
sg_play:
    anl a,#7Fh                  ; 5 bytes a sound
    mov r6,a
    rl a
    rl a
    add a,r6
    add a,#sndtab&0FFh
    mov r5,a
    mov a,#P1_VDC
    outl p1,a
    mov r0,#vdc_sound0
    mov r6,#3
sg_pat:
    mov a,r5
    movp a,@a
    movx @r0,a
    inc r0
    inc r5
    djnz r6,sg_pat
    mov a,r5
    movp a,@a
    mov @r1,a                   ; its frames (the vblank's from here)
    inc r5
    mov a,r5
    movp a,@a
    mov r0,#vdc_soundctrl
    movx @r0,a
    mov a,#P1_MAIN
    outl p1,a
    ret

P_2     macro                   ; the patterns, by period in bits
    db 0AAh, 0AAh, 0AAh
    endm
P_3     macro
    db 092h, 049h, 024h
    endm
P_4     macro
    db 0CCh, 0CCh, 0CCh
    endm
P_6     macro
    db 0E3h, 08Eh, 038h
    endm
P_8     macro
    db 0F0h, 0F0h, 0F0h
    endm
P_24    macro
    db 0FFh, 0F0h, 000h
    endm
FAST    equ 0A0h                ; on, the fast clock
SLOW    equ 080h

sndtab:                         ; pattern, frames, control
    P_4                         ; SELECT        983 Hz
    db 3, FAST|12
    P_4                         ; SAMPLE        246
    db 3, SLOW|12
    P_24                        ; ERROR         164
    db 8, FAST|15
    P_3                         ; BUY           1311
    db 4, FAST|12
    P_4                         ; ALERT         983, then 1311 (fixed.inc)
    db 40h|(8+SND_A2), FAST|15
    P_8                         ; AUCTION OVER  491
    db 6, FAST|12
    P_3                         ; TRADE         1311, a click
    db 2, FAST|10
    P_4                         ; EVENT         246
    db 8, SLOW|15
    P_6                         ; FOUNDER       655
    db 10, FAST|15
    P_24                        ; STEP          164, soft
    db 1, FAST|6
    P_2                         ; TICK          1966
    db 2, FAST|9
    P_3                         ; TOCK          1311
    db 2, FAST|9
