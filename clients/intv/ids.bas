' ===========================================================================
' ids.bas -- the numbers of the strings in text.bas (titles and messages).
' Kept apart from the strings so they can be included before any code uses
' them (IntyBASIC reads a CONST used before its definition as a variable).
' ===========================================================================

    ' Map titles (t_titles).
    CONST T_LANDGRANT = 0
    CONST T_LANDAUCTION = 1
    CONST T_DEVELOPMENT = 2
    CONST T_PRODUCTION = 3
    CONST T_TRANSPORT = 4
    CONST T_EVENT = 5           ' + colony event - 1 (pest .. fire)

    ' Messages (t_msgs).
    CONST M_NONE = 0
    CONST M_LANDING = 1
    CONST M_DEPART = 2
    CONST M_SELECT = 3
    CONST M_NONEW = 4
    CONST M_START = 5
    CONST M_COMPUTER = 6
    CONST M_TURN = 7            ' "<name>'S TURN" (name printed first)
    CONST M_TIMEOUT = 8
    CONST M_GAMBLE = 9
    CONST M_PRESSALL = 10
    CONST M_FOODSHORT = 11
    CONST M_EVENT = 12          ' + colony event - 1
    CONST M_PREVIOUS = 21
    CONST M_USAGE = 22
    CONST M_SPOILAGE = 23
    CONST M_PRODUCTION = 24
    CONST M_STOREHAS = 25
    CONST M_STORENO = 26
    CONST M_DECLARE = 27
    CONST M_AUCOVER = 28
    CONST M_CRITICAL = 29
    CONST M_SELLERDRY = 30
    CONST M_STOREDRY = 31
    CONST M_TRADED = 32
    CONST M_SOLD = 33
    CONST M_NOBIDS = 34
    CONST M_BIDSTART = 35
    CONST M_HIGHBID = 36
    CONST M_SALEPRICE = 37
    CONST M_MONEY = 38
    CONST M_NOMONEY = 39
    CONST M_NOMULES = 40
    CONST M_ASSAY = 41          ' + level 0..4
    CONST M_FORSALE = 46
    CONST M_NOTFORSALE = 47
    CONST M_RANAWAY = 48
    CONST M_CAUGHT = 49
    CONST M_SPECTATE = 50
    CONST M_WAITING = 51
    CONST M_READY = 52
    CONST M_COLLUDE = 53
    CONST M_LANDOFFICE = 54
    CONST M_NOSAMPLE = 55
    CONST M_OUTFIT = 56
    CONST M_SHIPBACK = 57
