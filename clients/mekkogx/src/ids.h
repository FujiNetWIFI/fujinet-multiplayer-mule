/*
 * ids.h -- the numbers of the strings in text.c (clients/intv/ids.bas).
 */
#ifndef IDS_H
#define IDS_H

/* titles */
#define T_LANDGRANT   0
#define T_LANDAUCTION 1
#define T_DEVELOPMENT 2
#define T_PRODUCTION  3
#define T_TRANSPORT   4
#define T_EVENT       5     /* + colony event - 1 (pest .. fire) */

/* messages */
#define M_NONE        0
#define M_LANDING     1
#define M_DEPART      2
#define M_SELECT      3
#define M_NONEW       4
#define M_START       5
#define M_COMPUTER    6
#define M_TURN        7     /* "<name>'S TURN" (name printed first) */
#define M_TIMEOUT     8
#define M_GAMBLE      9
#define M_PRESSALL    10
#define M_FOODSHORT   11
#define M_EVENT       12    /* + colony event - 1 */
#define M_PREVIOUS    21
#define M_USAGE       22
#define M_SPOILAGE    23
#define M_PRODUCTION  24
#define M_STOREHAS    25
#define M_STORENO     26
#define M_DECLARE     27
#define M_AUCOVER     28
#define M_CRITICAL    29
#define M_SELLERDRY   30
#define M_STOREDRY    31
#define M_TRADED      32
#define M_SOLD        33
#define M_NOBIDS      34
#define M_BIDSTART    35
#define M_HIGHBID     36
#define M_SALEPRICE   37
#define M_MONEY       38
#define M_NOMONEY     39
#define M_NOMULES     40
#define M_ASSAY       41    /* + level 0..4 */
#define M_FORSALE     46
#define M_NOTFORSALE  47
#define M_RANAWAY     48
#define M_CAUGHT      49
#define M_SPECTATE    50
#define M_WAITING     51
#define M_READY       52
#define M_COLLUDE     53
#define M_LANDOFFICE  54
#define M_NOSAMPLE    55
#define M_OUTFIT      56
#define M_SHIPBACK    57
#define M_ENDTURN     58
#define M_LOBBYHELP   59
#define M_LEAVEHELP   60
#define M_LEAVEASK    61

/* words */
#define W_SUMMARY     0
#define W_MONEY       1
#define W_LAND        2
#define W_GOODS       3
#define W_TOTAL       4
#define W_COLONY      5
#define W_FOUNDER     6
#define W_AUCTION     7
#define W_STATUS      8
#define W_SELL        9
#define W_BUY         10
#define W_UNITS       11
#define W_TRADED      12
#define W_MULE        13
#define W_TOURNAMENT  14
#define W_PRESSFOR    15
#define W_STARS       16
#define W_COMPUTER    17
#define W_OPEN        18
#define W_OVERALL     19

#endif
