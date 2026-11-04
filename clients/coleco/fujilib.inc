; fujilib.inc -- the client half of the FujiNet ColecoVision cartridge mailbox.
;
; Hand-mirrors fujinet-firmware/pico/coleco/firmware/include/fuji_mailbox.h;
; every equate is a console address (cart offset + 8000H). From
; fujinet-fujitzee/coleco/fujilib.inc, plus APPBANK's page select.
;
; The connector has no /RD or /WR, so both directions ride reads: a register
; write is a read of FNREGSEL+reg, then of FNREGDAT+value; a reply is ROM the
; cart repaints. A REGDATA read without a REGSEL just before it is ignored.
; The vblank NMI can't be masked and lands mid-transaction: harmless, since
; the handler reads nothing at F800H and up (an armed register waits; the TX
; stream only appends).
;
; RAM at 7800H-7FFFH would look like F800H-FFFFH to the cart (A15 is not on
; the connector): never touch it.

; ---- reply bytes (repainted ROM) --------------------------------------
FNRDATA EQU     0F800H          ; the whole reply, 1K
FNACKSQ EQU     0FC00H          ; echoes SEQ when the reply is ready
FNSTATS EQU     0FC01H          ; bit0 link up, bit1 busy
FNERR   EQU     0FC02H          ; fb_status_t of the last transaction
FNREPLY EQU     0FC03H          ; 06H ACK / 15H NAK
FNRXLL  EQU     0FC04H          ; reply length, low
FNRXLH  EQU     0FC05H          ; reply length, high
FNMAGF  EQU     0FC09H          ; 'F'
FNMAGN  EQU     0FC0AH          ; 'N'
FNPVER  EQU     0FC0BH          ; protocol version

; ---- hotspot pages ----------------------------------------------------
FNREGSEL EQU    0FD00H          ; read +reg: arm register
FNREGDAT EQU    0FE00H          ; read +val: armed register = val
FNDATA  EQU     0FF00H          ; read +val: append val to the TX stream
FNRSELP EQU     0FDH            ; their page bytes (zmac 1.3 has no HIGH)
FNRDATP EQU     0FEH
FNDATAP EQU     0FFH

; ---- APPBANK ------------------------------------------------------------
; A read of FNBKSEL+n maps image page n at C000H-DFFFH before the next
; fetch; the byte it returns is undefined. A console RESET leaves the page
; where it was (the cart never sees it): the start code selects page 0.
FNBKSEL EQU     0FD80H
FNBKLO  EQU     80H             ; FNBKSEL's low byte

; ---- register numbers -------------------------------------------------
FRDEVIC EQU     00H
FRCMD   EQU     01H
FRNPARM EQU     02H
FRDATRS EQU     05H
FRSEQ   EQU     10H

; ---- values -----------------------------------------------------------
FNDFUJI EQU     70H             ; FujiBus device id of the FujiNet itself
FCACK   EQU     06H             ; reply command: ACK
FCNAK   EQU     15H             ; reply command: NAK
FEOK    EQU     0               ; FNERR values
FENOLNK EQU     1
FETMOUT EQU     2

; ---- routines ---------------------------------------------------------

; FNREGWR: register C = A. Clobbers A,B,D,E.
FNREGWR:
        LD      B,A
        LD      D,FNRSELP
        LD      E,C
        LD      A,(DE)          ; arm the register
        LD      D,FNRDATP
        LD      E,B
        LD      A,(DE)          ; deliver the value
        RET

; FNTXBYT: A onto the TX stream. Clobbers A,D,E.
FNTXBYT:
        LD      D,FNDATAP
        LD      E,A
        LD      A,(DE)
        RET

; FNPARB: a 1-byte parameter A. Clobbers A,B,D,E.
FNPARB:
        LD      B,A
        LD      A,1             ; its size
        CALL    FNTXBYT
        LD      A,B
        JP      FNTXBYT

; FNTXSTR: the string at HL (to its NUL, not sent). Clobbers A,D,E,HL.
FNTXSTR:
        LD      A,(HL)
        OR      A
        RET     Z
        CALL    FNTXBYT
        INC     HL
        JR      FNTXSTR

; FNBEGIN: a transaction: A = device, E = command, L = parameters. Rewinds
; the TX stream; the caller then sends the parameters and the payload.
; Clobbers A,B,C,D,E,H,L.
FNBEGIN:
        PUSH    HL
        PUSH    DE
        PUSH    AF
        LD      C,FRDATRS
        XOR     A
        CALL    FNREGWR
        POP     AF
        LD      C,FRDEVIC
        CALL    FNREGWR
        POP     DE
        LD      A,E
        LD      C,FRCMD
        CALL    FNREGWR
        POP     HL
        LD      A,L
        LD      C,FRNPARM
        JP      FNREGWR

; FNCOMMIT: launch it and wait for the answer; B = ~2 s quanta. The sequence
; number is the cart's ACKSEQ + 1 (255 -> 1), never a counter of ours: a
; console RESET restarts us but not the cart. Carry on a timeout; else
; A = FNERR (Z: ok). Clobbers A,B,C,D,E,H,L.
FNCOMMIT:
        LD      A,(FNACKSQ)
        INC     A
        JR      NZ,FNCMS
        INC     A
FNCMS:  LD      H,A
        LD      C,FRSEQ
        CALL    FNREGWR
FNCMO:  LD      DE,0
FNCMI:  LD      A,(FNACKSQ)
        CP      H
        JR      Z,FNCMOK
        DEC     DE
        LD      A,D
        OR      E
        JR      NZ,FNCMI
        DJNZ    FNCMO
        SCF
        RET
FNCMOK: LD      A,(FNERR)
        OR      A
        RET

; FNCHECK: Z if a FujiNet cartridge is there. Clobbers A.
FNCHECK:
        LD      A,(FNMAGF)
        CP      'F'
        RET     NZ
        LD      A,(FNMAGN)
        CP      'N'
        RET
