; dvturn.inc -- the development's turn, for its walking pages (11, 12).

; dv_turn: the turn in the state (mekkogx develop_state): DV_SEAT,
; DV_FLAGS, what we lead (the server's word wins); DV_ST: ours to begin
; (DV_PROMPT), ours under way (DV_WALK: begun, into town, where a turn
; starts), another's or ours over (DV_WAIT: back to the map). Its words
; are page 13's. Clobbers A, r0, r2-r6.
dv_turn:
    STB ST_TURN+TU_SEAT
    mov r0,#DV_SEAT
    movx @r0,a
    mov r5,a
    STB ST_TURN+TU_FLAGS
    mov r0,#DV_FLAGS
    movx @r0,a
    mov r4,a
    anl a,#TF_HASMULE
    mov r0,#DV_MULE
    movx @r0,a
    mov r0,#MYSEAT
    movx a,@r0
    xrl a,r5
    jnz dt_wait
    mov a,r4
    anl a,#TF_STARTED
    mov r6,#DV_PROMPT
    jz dt_set
    mov a,r4
    anl a,#TF_ENDED
    jnz dt_wait
    mov r0,#DV_ST               ; under way: begun just now, into town
    movx a,@r0
    xrl a,#DV_WALK
    jz dt_x
    mov a,#DV_WALK
    movx @r0,a
    mov a,#0FFh                 ; no sample, no land office; mid-street
    mov r0,#DV_SAMPLE
    movx @r0,a
    clr a
    mov r0,#DV_OFFICE
    movx @r0,a
    mov r0,#DV_CNT
    movx @r0,a
    mov r0,#DV_X
    mov a,#76
    movx @r0,a
    inc r0
    mov a,#108
    movx @r0,a
    mov r0,#DV_TOWN
    mov a,#1
    movx @r0,a
dt_x:
    ret
dt_wait:
    mov r6,#DV_WAIT
dt_set:
    mov r0,#DV_ST
    mov a,r6
    movx @r0,a
    clr a                       ; (and on the map)
    mov r0,#DV_TOWN
    movx @r0,a
    ret
