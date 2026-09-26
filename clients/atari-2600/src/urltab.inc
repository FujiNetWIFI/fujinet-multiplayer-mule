; urltab.inc -- url.inc's pointer tables, which are DATA (their bytes can
; be $81 or $91): included after the bank's DSTART.
RTLO:   DB      (R01)&$FF,(R01)&$FF,(R02)&$FF,(R03)&$FF,(R04)&$FF,(R05)&$FF,(R06)&$FF,(R07)&$FF,(R08)&$FF,(R09)&$FF,(R10)&$FF
        DB      (R11)&$FF,(R12)&$FF,(R13)&$FF,(R14)&$FF,(R15)&$FF,(R16)&$FF,(R17)&$FF,(R18)&$FF,(R19)&$FF,(R20)&$FF,(R21)&$FF,(R22)&$FF
RTHI:   DB      (R01)>>8,(R01)>>8,(R02)>>8,(R03)>>8,(R04)>>8,(R05)>>8,(R06)>>8,(R07)>>8,(R08)>>8,(R09)>>8,(R10)>>8
        DB      (R11)>>8,(R12)>>8,(R13)>>8,(R14)>>8,(R15)>>8,(R16)>>8,(R17)>>8,(R18)>>8,(R19)>>8,(R20)>>8,(R21)>>8,(R22)>>8
; What follows the route: 0 nothing, 1 NARG in decimal, 2 push, 3 declare.
PWLO:   DB      (PW0)&$FF,(PW1)&$FF,(PW2)&$FF
PWHI:   DB      (PW0)>>8,(PW1)>>8,(PW2)>>8
