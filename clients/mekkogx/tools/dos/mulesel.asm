; mulesel.asm -- MULESEL.COM, the last line of the M.U.L.E. disk's
; AUTOEXEC.BAT: types MULEJR (a PCjr: the build with only its 16-colour
; mode, src/msdos/machine.h) or MULE into the BIOS's keyboard buffer and
; ends, so COMMAND.COM runs the game as if typed, with nothing of this
; left in memory and nothing of the batch file left to run after it (the
; way src/msdos/lobby.c goes back to the lobby).
.8086
                NAME    mulesel

_TEXT           SEGMENT BYTE PUBLIC 'CODE'
                ASSUME  CS:_TEXT, DS:_TEXT, ES:NOTHING
                ORG     100h
start:          cld
                mov     si, offset pc
                mov     ax, 0F000h
                mov     es, ax
                cmp     byte ptr es:[0FFFEh], 0FDh      ; the PCjr's model byte
                jne     sel
                mov     si, offset jr
sel:            mov     ax, 40h
                mov     es, ax
                mov     di, 1Eh                 ; the buffer, 001E..003D
                cli
                mov     es:[1Ah], di            ; its head
key:            lodsb
                or      al, al
                jz      done
                xor     ah, ah
                cmp     al, 0Dh
                jne     put
                mov     ah, 1Ch                 ; ENTER's scan code
put:            stosw
                jmp     key
done:           mov     es:[1Ch], di            ; its tail
                sti
                mov     ax, 4C00h
                int     21h
pc              DB      'MULE', 0Dh, 0
jr              DB      'MULEJR', 0Dh, 0
_TEXT           ENDS
                END     start
