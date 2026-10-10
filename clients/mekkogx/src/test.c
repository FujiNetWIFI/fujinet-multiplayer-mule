/*
 * test.c -- an offline test mode for tools/a2sim (clients/intv/test.bas).
 *
 * When the harness pokes $5A into test_hook[0] during the title, the game
 * skips FujiNet and fakes a state: round 1 on a generated map. test_hook[1]
 * picks the phase + 1 (0: develop), [2] the colony event / auction stage /
 * the seat whose turn it is, [3] the player event. Actions are answered
 * locally, so walking, the town, events and animations can be checked in
 * seconds. The fake state is built by teststart.c; this answers actions.
 */
#include <string.h>
#include "mule.h"

/* Answer an action as the server would, roughly, and age the clocks. */
void test_poll(void)
{
    uint16_t w = since_rx(), v;
    net_have = 1;
    act_sent = act;
    st.h.err = 0;
    switch (act) {
    case ACT_BUYMULE: st.turn.flags = TF_STARTED | TF_HASMULE; break;
    case ACT_RETURN:  st.turn.flags = TF_STARTED; break;
    case ACT_OUTFIT:  st.turn.carrying = act_arg; break;
    case ACT_INSTALL:
        st.turn.flags = TF_STARTED;
        st.map.owner[act_arg] = 0;
        st.map.mule[act_arg] = st.turn.carrying;
        break;
    case ACT_ASSAY:   st.h.result = 3; break;
    case ACT_PUB:     st.turn.flags = TF_STARTED | TF_ENDED; st.h.result = 75; break;
    case ACT_CLAIM:   st.grant.claimed[0] = act_arg; st.map.owner[act_arg] = 0; break;
    }
    v = st.turn.left;
    st.turn.left = (st.h.phase == PHASE_DEVELOP && v > w) ? v - w : 0;
    if (st.h.phase != PHASE_DEVELOP || st.turn.left)
        st.h.elapsed += w;
    st.h.t += w;                /* the server's clock: the auction lines step on it */
    st.turn.elapsed += w;
    if (st.h.phase == PHASE_GRANT) {
        /* as the server's: the cursor waits (-1) for the ship, then steps
         * every 12 (next: jiffies to its next step) */
        v = w;
        if (st.grant.cursor < 0) {
            if (st.grant.next > v) {
                st.grant.next -= v;
                v = 0;
            } else {
                v -= st.grant.next;
                st.grant.cursor = 0;
                st.grant.next = 12;
            }
        }
        if (st.grant.cursor >= 0) {
            v += (uint16_t)st.grant.cursor * 12 + 12 - st.grant.next;
            st.grant.cursor = v / 12 > 44 ? 45 : (int8_t)(v / 12);
            st.grant.next = 12 - v % 12;
        }
    }
    act = ACT_NONE;
    state_arrived();
}
