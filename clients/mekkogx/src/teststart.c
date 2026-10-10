/*
 * teststart.c -- the offline test mode's fake state (see test.c). Only
 * needed at start-up, so it can live in the boot overlay.
 */
#include <string.h>
#include "mule.h"

void test_start(uint8_t ph, uint8_t arg, uint8_t pev)
{
    uint8_t i, mp, c;
    MulePlayer *p;

    test_mode = 1;
    memset(&st, 0, sizeof(st));
    st.h.magic = 'M';
    st.h.version = MULE_PROTO_VERSION;
    st.h.msgType = MSG_STATE;
    st.h.phase = ph ? ph - 1 : PHASE_DEVELOP;
    st.h.round = 1;
    st.h.mySeat = 0;
    st.h.countdown = -1;
    for (i = 0; i < 4; ++i) {
        p = &st.players[i];
        p->name[0] = 'P';
        p->name[1] = 'L';
        p->name[2] = '0' + i;
        /* the lobby and summary take arg as a species offset: 1 shows the other four */
        p->species = (1 + i * 2 + (ph == PHASE_LOBBY + 1 || ph == PHASE_SUMMARY + 1 ? arg : 0)) & 7;
        p->color = i;
        p->money = 1000;
        p->rank = i + 1;
        p->goods[0] = 1 + i * 2;        /* food 1/3/5/7: short, even, over */
        p->prod[0] = 3 + i;
        p->spoil[0] = i;
        p->score = 5000 - i * 700;
    }
    st.players[3].flags = PF_AI;
    /* the river down column 4, the store in the middle, mountains either side */
    for (mp = 0; mp < NUM_PLOTS; ++mp) {
        c = TERRAIN_PLAIN;
        if (mp % 9 == 4)
            c = TERRAIN_RIVER;
        if (mp == STORE_PLOT)
            c = TERRAIN_STORE;
        if (mp % 9 == (mp / 9) % 4)
            c = TERRAIN_MOUNTAIN + ((mp / 9) % 3 + 1) * 4;
        if (mp % 9 == 5 + (mp / 9) % 4)
            c = TERRAIN_MOUNTAIN + ((mp / 9 + 1) % 3 + 1) * 4;
        st.map.terrain[mp] = c;
        st.map.owner[mp] = -1;
        st.map.mule[mp] = -1;
        st.map.crystite[mp] = -1;
    }
    st.map.owner[10] = 0; st.map.mule[10] = GOOD_FOOD; st.map.prod[10] = 5;
    st.map.owner[12] = 1; st.map.mule[12] = GOOD_ENERGY; st.map.prod[12] = 3;
    st.map.owner[30] = 3; st.map.mule[30] = GOOD_ENERGY;
    st.map.owner[20] = 0; st.map.mule[20] = GOOD_SMITHORE; st.map.prod[20] = 8;
    st.map.owner[14] = 2;
    st.map.owner[34] = 2; st.map.mule[34] = GOOD_CRYSTITE; st.map.prod[34] = 2;
    st.store.mules = 14;
    st.store.mulePrice = 100;
    for (i = 0; i < 4; ++i) {
        st.store.price[i] = 20 + i * 20;
        st.store.stock[i] = 8;
    }
    /* a turn: ours (or seat arg's), started, 2828 jiffies, a wampus early on */
    st.turn.seat = arg & 3;
    st.turn.flags = TF_STARTED;
    st.turn.length = st.turn.left = 2828;
    st.turn.pTU = 28;
    st.turn.carrying = -1;
    st.turn.event = pev;
    st.turn.nWampus = 1;
    st.turn.wampus[0].plot = 0;
    st.turn.wampus[0].to = 2815;
    /* production / colony event */
    st.colony.event = arg;
    st.colony.plot = 10;
    st.colony.total = 123456;
    st.colony.rating = 4;
    /* auctions */
    st.auction.kind = 2;
    st.auction.step = 1;
    if (st.h.phase == PHASE_LANDAUCTION) {
        st.auction.kind = 1;
        st.auction.step = 4;
        st.auction.start = 240;
        st.auction.plot = 13;
    }
    st.auction.storeBuy = 30;
    st.auction.storeSell = 65;
    for (i = 0; i < 4; ++i) {
        st.auction.role[i] = ROLE_BUYER;
        st.auction.pos[i] = 20 + i * 5;
        st.auction.target[i] = 40 + i * 5;
    }
    st.auction.role[1] = ROLE_SELLER;
    st.auction.ticks = 70;
    st.auction.stage = st.h.phase == PHASE_DECLARE || st.h.phase == PHASE_TRADING ||
                       st.h.phase == PHASE_LANDAUCTION ? arg : 0;
    st.auction.hi = 66;
    st.auction.lo = 20;
    st.grant.cursor = -1;           /* the ship leaves first (the server's */
    st.grant.step = 12;             /* GrantIntro + LandingDelay) */
    st.grant.next = 186 + 726;
    for (i = 0; i < 4; ++i)
        st.grant.claimed[i] = -1;
    strcpy(sc_name, "PL0");
    strcpy(sc_table, "test");
}
