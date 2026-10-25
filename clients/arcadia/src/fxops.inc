; fxops.inc -- the sprite scripts' bytes (p_fx.inc runs them; the
; development writes a computer's walk into RAM-H).

FO_END   EQU    0
FO_POS   EQU    1
FO_MOVE  EQU    2
FO_WAIT  EQU    3
FO_LOOK  EQU    4
FO_FRAME EQU    5
FO_BG    EQU    6
FO_HIDE  EQU    7
FO_SIG   EQU    8
FO_LOOP  EQU    9
FO_COL   EQU    10
FO_N     EQU    11

FK_SINGLE EQU   1
FK_SHIP  EQU    3
FKHID    EQU    $80             ; (in KIND) hidden

XB       MACRO  px              ; a script's x
        DB      (px)+64
         ENDM
YB       MACRO  ln              ; and line
        DB      (ln)+48
         ENDM
AIX      MACRO  adr             ; a picture's index
        DB      ((adr)-$0800)>>3
         ENDM
