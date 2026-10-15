; ---------------------------------------------------------------------------
; test.inc -- the offline test mode's answers (mekkogx test.c test_poll).
;
; Resident: it ends in STARRIV, which maps the phase's page under whatever
; called it. The fake state itself is built in page 0 (TESTST).
; ---------------------------------------------------------------------------

; TESTPOLL -- answer an action as the server would, roughly, and age the
;   clocks by the frames since the last state.
TESTPOLL: ENTER
        LDW 0, 1, FRAMES        ; w = frames since the last state
        LDW 2, 3, RXFRAME
        PI SUB16
        STW 0, 1, GT
        LIS 1
        STB NETHAVE
        LDB ACT
        STB ACTSENT
        CLR
        STB ST+H_ERR
        LDB ACTARG
        LR 4,A
        LDB ACT
        CI ACT_BUYM
        BNZ TP1
        SETB ST+T_FLAGS, TF_START|TF_MULE
        JMP TP9
TP1:    CI ACT_RETN
        BNZ TP2
        SETB ST+T_FLAGS, TF_START
        JMP TP9
TP2:    CI ACT_OUTF
        BNZ TP3
        LR A,4
        STB ST+T_CARRY
        JMP TP9
TP3:    CI ACT_INST
        BNZ TP4
        SETB ST+T_FLAGS, TF_START
        DCI ST+M_OWNER
        LR A,4
        ADC
        CLR
        ST
        LDB ST+T_CARRY
        LR 5,A
        DCI ST+M_MULE
        LR A,4
        ADC
        LR A,5
        ST
        BR TP9
TP4:    CI ACT_ASSAY
        BNZ TP5
        SETB ST+H_RES, 3
        CLR
        ST
        BR TP9
TP5:    CI ACT_PUB
        BNZ TP6
        SETB ST+T_FLAGS, TF_START|TF_ENDED
        SETB ST+H_RES, 75
        CLR
        ST
        BR TP9
TP6:    CI ACT_CLAIM
        BNZ TP9
        LR A,4
        STB ST+GR_CLM
        DCI ST+M_OWNER
        LR A,4
        ADC
        CLR
        ST
TP9:    ; turn time left: (developing and left > w) ? left - w : 0
        LDW 0, 1, ST+T_LEFT
        LDW 2, 3, GT
        LDB ST+H_PHASE
        CI PH_DEV
        BNZ TPZ
        PI GE16
        BNC TPZ
        PI SUB16
        BR TPS
TPZ:    CLR
        LR 0,A
        LR 1,A
TPS:    STW 0, 1, ST+T_LEFT
        LDB ST+H_PHASE          ; the phase's clock runs, but not on a turn
        CI PH_DEV               ; whose time is up
        BNZ TPE
        LR A,0
        NI 0FFH
        BNZ TPE
        LR A,1
        NI 0FFH
        BZ TPT
TPE:    LDW 0, 1, ST+H_ELAP
        LDW 2, 3, GT
        PI ADD16
        STW 0, 1, ST+H_ELAP
TPT:    LDW 2, 3, GT            ; the server's clock (the auction lines step
        LDW 0, 1, ST+H_T        ; on it) and the turn's
        PI ADD16
        STW 0, 1, ST+H_T
        LDW 0, 1, ST+T_ELAP
        PI ADD16
        STW 0, 1, ST+T_ELAP
        LDB ST+H_PHASE
        CI PH_GRANT
        JNZ TPD
        ; the grant's cursor waits (-1) for the ship, then steps every 12
        ; (next: jiffies to its next step)
        LDW 0, 1, GT            ; v
        LDB ST+GR_CUR
        NI 080H
        BZ TPG2
        LDW 2, 3, ST+GR_NEXT
        PI GE16                 ; v >= next: the cursor starts
        BC TPG1
        LR A,0                  ; next -= v
        LR 4,A
        LR A,1
        LR 5,A
        LDW 0, 1, ST+GR_NEXT
        LR A,4
        LR 2,A
        LR A,5
        LR 3,A
        PI SUB16
        STW 0, 1, ST+GR_NEXT
        BR TPD
TPG1:   PI SUB16                ; v -= next
        SETB ST+GR_CUR, 0
        SETB ST+GR_NEXT, 12
        CLR
        ST
TPG2:   STW 0, 1, GT            ; v += cursor * 12 + 12 - next
        LDB ST+GR_CUR
        LR 0,A
        CLR
        LR 1,A
        LI 12
        LR 2,A
        PI MUL8
        LDW 2, 3, GT
        PI ADD16
        LI 12
        LR 2,A
        CLR
        LR 3,A
        PI ADD16
        LDW 2, 3, ST+GR_NEXT
        PI SUB16
        LI 12                   ; cursor = v / 12 (45 past the last plot)
        LR 2,A
        PI DIV8
        LR A,3
        STB GT+2                ; v % 12
        LR A,1
        NI 0FFH
        BNZ TPG3
        LR A,0
        CI 44
        BC TPG4
TPG3:   LI 45
TPG4:   STB ST+GR_CUR
        LDB GT+2                ; next = 12 - v % 12
        COM
        INC
        AI 12
        STB ST+GR_NEXT
        CLR
        ST
TPD:    CLR
        STB ACT
        PI STARRIV
        LEAVE
