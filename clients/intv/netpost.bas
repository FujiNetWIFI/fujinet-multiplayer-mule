' ===========================================================================
' netpost.bas -- a mailbox transaction posted, and its answer looked for,
' without waiting (netstep.bas; fn_transact in fujinet.bas waits). In
' segment 3, for room.
' ===========================================================================

' fn_post: fn_transact without the wait (mb_seq is the echo to look for).
fn_post: PROCEDURE
    POKE (FN_DEVICE), mb_dev
    POKE (FN_CMD), mb_cmd
    POKE (FN_NPARAM), mb_nparam
    POKE (FN_TXLEN_LO), #fn_txlen AND 255
    POKE (FN_TXLEN_HI), #fn_txlen / 256
    mb_seq = (PEEK(FN_ACKSEQ) AND 255) + 1
    IF mb_seq = 0 THEN mb_seq = 1
    POKE (FN_SEQ), mb_seq
    #fn_t = 0
END

' fn_check: fn_ok 255 while the cart hasn't answered (900 looks at most),
' else as fn_transact leaves it.
fn_check: PROCEDURE
    IF (PEEK(FN_ACKSEQ) AND 255) <> mb_seq THEN
        #fn_t = #fn_t + 1
        fn_ok = 255
        IF #fn_t >= 900 THEN fn_ok = 0 : #mb_err = 0
        RETURN
    END IF
    fn_ok = 1
    IF (PEEK(FN_REPLY_CMD) AND 255) <> FUJICMD_ACK THEN fn_ok = 0 : #mb_err = (PEEK(FN_ERR) AND 255)
END
