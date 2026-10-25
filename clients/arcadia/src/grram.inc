; grram.inc -- the land grant's bookkeeping (p_grant.inc; autoplay reads it).

        ZONE    UNIONL,UNIONLE
        VAR     GCUR,1          ; the cursor's plot (255 none, 45 done)
        VAR     GSTEP,1         ; jiffies a step
        VAR     GSHOW,1         ; 1: the cursor shows
        VAR     GDUEH,1         ; the frame its next step is due
        VAR     GDUEL,1
        VAR     GLIM,1          ; the furthest plot it may step to
        VAR     GSTEPF,1        ; the frame (low byte) of its last step
        VAR     GDEPART,1       ; 1: the ship is leaving first
        VAR     GCLAIMED,1      ; 1: we claimed this round
        VAR     GPRESS,1        ; the cursor's plot when the button went down (255)
        VAR     GWAS,1          ; the plot before its last step (255)
        VAR     GWANT,1         ; the plot we claimed and wait on (255)
        VAR     GPARK,1         ; 1: turned down early: sent again when it's there

