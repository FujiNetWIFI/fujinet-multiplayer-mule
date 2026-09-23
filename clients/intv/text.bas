' ===========================================================================
' text.bas -- every string the game prints.
'
' Wording is the Atari's, read off the original screens (docs/ANIMATION.md
' lists how). Strings are length-prefixed DATA ("DATA 5, "HELLO""), stored
' by IntyBASIC as ASCII - 32, the GROM card number. A '%' is replaced by a
' number when printed (see msg_show). Long lines scroll along the message
' row the way the Atari's do.
' ===========================================================================

    ' Map titles (row 0).
t_titles:
    DATA 10, "LAND GRANT"
    DATA 15, "LAND AUCTION #%"
    DATA 14, "DEVELOPMENT #%"
    DATA 13, "PRODUCTION #%"
    DATA 14, "TRANSPORT SHIP"
    DATA 11, "PEST ATTACK"
    DATA 11, "PIRATE SHIP"
    DATA 15, "ACID RAIN STORM"
    DATA 11, "PLANETQUAKE"
    DATA 16, "SUNSPOT ACTIVITY"
    DATA 9, "METEORITE"
    DATA 9, "RADIATION"
    DATA 13, "FIRE IN STORE"
    DATA 14, "TRANSPORT SHIP"

    ' Messages (the map's bottom row, or the message row of other screens).
t_msgs:
    DATA 0, ""
    DATA 35, "YOU'RE LANDING ON THE PLANET IRATA."
    DATA 35, "THE SHIP WILL BE BACK IN 12 MONTHS."
    DATA 35, "PRESS YOUR BUTTON TO SELECT A PLOT."
    DATA 22, "NO NEW PLOTS FOR SALE."
    DATA 32, "PRESS YOUR STICK BUTTON TO START"
    DATA 31, "THE COMPUTER IS TAKING HIS TURN"
    DATA 7, "'S TURN"
    DATA 16, "TIME HAS RUN OUT"
    DATA 16, "WON $% GAMBLING!"
    DATA 34, "PRESS ALL PLAYER BUTTONS TO GO ON."
    DATA 29, "FOOD SHORTAGE DECREASES TIME."
    ' colony events 1..9
    DATA 30, "PESTS RUIN A PLOT'S FOOD CROP!"
    DATA 27, "PIRATES TAKE ALL CRYSTITE !"
    DATA 31, "FOOD OUTPUT UP, ENERGY REDUCED."
    DATA 29, "MINING OUTPUT IS CUT IN HALF!"
    DATA 26, "ENERGY OUTPUT IS INCREASED"
    DATA 31, "A HUGE CRYSTITE DEPOSIT IS MADE"
    DATA 16, "MULE GOES CRAZY!"
    DATA 35, "ALL THE STOCK IN THE STORE IS LOST!"
    DATA 17, "THE SHIP IS BACK!"
    ' per-good status and auctions
    DATA 15, "PREVIOUS AMOUNT"
    DATA 5, "USAGE"
    DATA 8, "SPOILAGE"
    DATA 10, "PRODUCTION"
    DATA 17, "STORE HAS % UNITS"
    DATA 18, "STORE HAS NO UNITS"
    DATA 40, "PUSH STICK TO DECLARE AS BUYER OR SELLER"
    DATA 15, "AUCTION IS OVER"
    DATA 25, "SELLER AT CRITICAL LEVEL!"
    DATA 17, "SELLER'S RUN DRY!"
    DATA 18, "STORE HAS RUN DRY!"
    DATA 14, "UNITS TRADED %"
    DATA 13, "PLOT IS SOLD!"
    DATA 16, "NO BIDS TENDERED"
    DATA 16, "BIDS START AT $%"
    DATA 14, "HIGH BID IS $%"
    DATA 14, "SALE PRICE: $%"
    DATA 15, "MONEY EQUALS $%"
    DATA 16, "NOT ENOUGH MONEY"
    DATA 17, "NO M.U.L.E.S LEFT"
    DATA 18, "ASSAY: NO CRYSTITE"
    DATA 19, "ASSAY: LOW CRYSTITE"
    DATA 13, "ASSAY: MEDIUM"
    DATA 20, "ASSAY: HIGH CRYSTITE"
    DATA 16, "ASSAY: VERY HIGH"
    DATA 20, "PLOT LISTED FOR SALE"
    DATA 17, "LISTING WITHDRAWN"
    DATA 21, "YOUR M.U.L.E. RAN OFF"
    DATA 22, "YOU CAUGHT THE WAMPUS!"
    DATA 13, "WATCHING GAME"
    DATA 19, "WAITING FOR PLAYERS"
    DATA 5, "READY"
    DATA 9, "COLLUSION"
    DATA 30, "PRESS BUTTON ON A PLOT TO SELL"
    DATA 19, "TAKE A SAMPLE FIRST"
    DATA 18, "OUTFIT M.U.L.E. $%"
    DATA 17, "THE SHIP IS BACK!"

' Goods, species, words.
t_goods:
    DATA 4, "FOOD"
    DATA 6, "ENERGY"
    DATA 8, "SMITHORE"
    DATA 8, "CRYSTITE"
t_species:
    DATA 8, "MECHTRON"
    DATA 8, "GOLLUMER"
    DATA 6, "PACKER"
    DATA 7, "BONZOID"
    DATA 8, "SPHEROID"
    DATA 7, "FLAPPER"
    DATA 7, "LEGGITE"
    DATA 8, "HUMANOID"

' Summary and other screens.
t_words:
    DATA 16, "STATUS SUMMARY #"    ' 0
    DATA 5, "MONEY"                ' 1
    DATA 4, "LAND"                 ' 2
    DATA 5, "GOODS"                ' 3
    DATA 5, "TOTAL"                ' 4
    DATA 6, "COLONY"               ' 5
    DATA 13, "FIRST FOUNDER"       ' 6
    DATA 9, "AUCTION #"            ' 7
    DATA 8, "STATUS #"             ' 8
    DATA 4, "SELL"                 ' 9
    DATA 3, "BUY"                  ' 10
    DATA 5, "UNITS"                ' 11
    DATA 6, "TRADED"               ' 12
    DATA 8, "M.U.L.E."             ' 13
    DATA 10, "TOURNAMENT"          ' 14
    DATA 9, "PRESS FOR"            ' 15
    DATA 3, "***"                  ' 16
    DATA 8, "COMPUTER"             ' 17
    DATA 4, "OPEN"                 ' 18
    DATA 9, "OVERALL, "            ' 19
