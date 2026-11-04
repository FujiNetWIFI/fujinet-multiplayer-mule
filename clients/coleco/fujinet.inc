; fujinet.inc -- N: device and appkey commands.
;
; The N: command values are the family's (netcat, 5cardstud, battleship);
; the canonical list is fujinet-firmware include/fujiCommandID.h. The
; mailbox transport itself is fujilib.inc.

; ---- Network device (N:) ----------------------------------------------
NETDEV  EQU     71H             ; N1:
NCOPEN  EQU     4FH             ; 'O'
NCCLOSE EQU     43H             ; 'C'
NCREAD  EQU     52H             ; 'R'
NCSTAT  EQU     53H             ; 'S'

NMHTTPG EQU     12              ; open mode: HTTP GET
NTRNONE EQU     0               ; translation: none

; NET_STATUS reply: 4 bytes in slice 0.
NSAVLO  EQU     0               ; bytes waiting, low
NSAVHI  EQU     1               ; bytes waiting, high
NSCONN  EQU     2               ; connected flag
NSDEVST EQU     3               ; nDevStatus_t: 1 OK, 136 EOF
NDSOK   EQU     1
NDSEOF  EQU     136

; ---- Fuji device appkeys ----------------------------------------------
; The FujiNet lobby's: creator 1, app 1. Key 0 is the player's name (every
; FujiNet game shares it), key 10 the table's URL. The open payload is six
; RAW tx bytes {creator lo, hi, app, key, mode, reserved}; omitting the
; reserved byte hangs the firmware (gcal/intv found it). The read reply is
; a 2-byte LE length ahead of the payload.
FCOKEY  EQU     0DCH            ; OPEN_APPKEY
FCCKEY  EQU     0DBH            ; CLOSE_APPKEY
FCRKEY  EQU     0DDH            ; READ_APPKEY
FCWKEY  EQU     0DEH            ; WRITE_APPKEY
AKCRLO  EQU     01H
AKCRHI  EQU     00H
AKAPP   EQU     1
AKNAME  EQU     0
AKROOM  EQU     10
AKMRD   EQU     0
AKMWR   EQU     1
