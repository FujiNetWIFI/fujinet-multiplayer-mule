/*
 * mule.h -- what every part of the (platform neutral) game shares.
 *
 * The server runs the game; the client polls /state?bin=1 (MuleState, see
 * ../mule_proto.h), draws it the way the Atari 800 original did, sends the
 * player's moves, and plays every animation itself at the Atari's speed.
 * This is a port of the Intellivision client (clients/intv); the comments
 * name the .bas routine each piece comes from where that helps.
 *
 * All times are jiffies (1/60 s). Positions are the Atari's: x in colour
 * clocks, y in scanlines (see gfx.h).
 */
#ifndef MULE_H
#define MULE_H

#include <stdint.h>
#include "mule_proto.h"
#include "plat.h"
#include "gfx.h"
#include "input.h"
#include "sound.h"

/* ---- the state -------------------------------------------------------- */
extern MuleState st;            /* the last good state */
extern MuleState prev;          /* the state drawn last (for redraw diffs) */
extern uint8_t phase, rnd, my_seat, last_phase, last_round, st_ok;
extern uint16_t rx_pe, rx_frame;    /* phase-elapsed and frame at the last state */
extern uint16_t ph_t0;          /* frame a phase's own animation started */
#define pe_now() ((uint16_t)(rx_pe + (uint16_t)(frame_count - rx_frame)))
#define since_rx() ((uint16_t)(frame_count - rx_frame))

#define PL(s)      (st.players[s])
#define PLCOL(s)   (pl_col[st.players[s].color & 3])
#define OWNER(p)   ((uint8_t)st.map.owner[p])
#define MULEOF(p)  ((uint8_t)st.map.mule[p])
#define NOBODY     255
extern const uint8_t pl_col[4];

/* ---- what is on the screen -------------------------------------------- */
#define SCR_NONE    0
#define SCR_TEXT    1
#define SCR_MAP     2
#define SCR_TOWN    3
#define SCR_AUCTION 4
#define SCR_SUMMARY 5
#define SCR_FOUNDER 6
extern uint8_t scr_now;

/* what the per-frame handler animates */
#define AN_NONE    0
#define AN_WALK    1
#define AN_GRANT   2
#define AN_AUCTION 3
extern uint8_t anim;

/* ---- the network (net.c) ---------------------------------------------- */
#define ACT_NONE     0
#define ACT_STATE    1
#define ACT_READY    2
#define ACT_UNREADY  3
#define ACT_SPECIES  4
#define ACT_CLAIM    5
#define ACT_PUSH     6      /* arg: 0 hold, 1 up, 2 down */
#define ACT_DECLARE  7      /* arg: 0 sit out, 1 buyer, 2 seller */
#define ACT_COLLUDE  8
#define ACT_BEGIN    9
#define ACT_BUYMULE  10
#define ACT_OUTFIT   11     /* arg: GOOD_* */
#define ACT_INSTALL  12     /* arg: plot */
#define ACT_RETURN   13
#define ACT_ASSAY    14     /* arg: plot */
#define ACT_SELL     15     /* arg: plot */
#define ACT_UNSELL   16     /* arg: plot */
#define ACT_WAMPUS   17
#define ACT_PUB      18
#define ACT_END      19
#define ACT_LEAVE    20

extern uint8_t act, act_sent, net_have, net_fail, poll_wait, test_mode;
extern uint16_t act_arg;
extern uint8_t au_traded;       /* units traded as shown (sent with pushes) */
extern char sc_name[9], sc_endpt[66], sc_table[9];

void act_send(uint8_t a, uint16_t arg);     /* queue an action, poll now */
uint8_t net_busy(void);
void net_begin(void);           /* start the queued action (or a poll) */
uint8_t net_step(void);         /* advance; 1 when a reply is in (net_have) */
uint8_t net_sync(void);         /* begin + step until done (boot, leaving) */
uint8_t net_detect(void);
uint8_t net_tables(uint8_t *buf, uint8_t max);  /* /tables: count, entries in buf */
void appkey_load(void);         /* name and table from the lobby's appkeys */
void appkey_save_room(void);
void appkey_clear_room(void);

/* ---- text (text.c) ---------------------------------------------------- */
#include "ids.h"
extern char txt[128];           /* the message being shown */
extern uint8_t msg_row, msg_col, msg_scroll;
const char *str_title(uint8_t id);
const char *str_msg(uint8_t id);
const char *str_word(uint8_t id);
const char *str_good(uint8_t id);
const char *str_species(uint8_t id);
const char *str_pevent(uint8_t id);
const char *str_rating(uint8_t id);
extern const uint8_t pevent_x[22];
uint8_t text_expand(char *dst, const char *src, uint16_t v1, uint16_t v2);
void msg_show(uint8_t id, uint16_t v1, uint16_t v2);
void msg_start(uint8_t len);    /* txt holds len characters */
void msg_tick(void);
void msg_clear(void);
void title_show(uint8_t id, uint16_t v);
void text_center(uint8_t ty, const char *s, uint8_t col);
void big_center(uint8_t ty, const char *s, uint8_t col);
void put_num(uint8_t tx, uint8_t ty, uint16_t v, uint8_t w, uint8_t col);  /* right aligned */
void big_num(uint8_t tx, uint8_t ty, uint16_t v, uint8_t w, uint8_t col);
void big_num32(uint8_t tx, uint8_t ty, uint32_t v, uint8_t w, uint8_t col);
uint8_t fmt_num(char *dst, uint16_t v);

/* ---- the map (map.c) -------------------------------------------------- */
extern uint8_t map_dots, map_blink, map_blink_on;
void map_enter(void);
void map_draw_plot(uint8_t p);
void map_draw_all(void);
void map_update(void);
extern uint8_t bar_x, bar_top, bar_h, bar_col, bar_last;
void bar_draw(uint8_t px);

/* ---- sprite scripts (fx.c) -------------------------------------------- */
#include "fx.h"

/* ---- phases ----------------------------------------------------------- */
void state_arrived(void);
void poll_rate(void);

void lobby_enter(void);
void lobby_state(void);
void lobby_tick(void);
void grant_enter(void);
void grant_state(void);
void grant_tick(void);
void grant_isr(void);
void landauc_enter(void);
void goods_enter(void);
void auction_state(void);
void auction_tick(void);
void auction_isr(void);
void develop_enter(void);
void develop_state(void);
void develop_tick(void);
void walk_isr(void);
void production_enter(void);
void production_tick(void);
void summary_enter(void);
void summary_state(void);
void summary_tick(void);
void gameover_enter(void);
void gameover_tick(void);

void boot(void);
void game_start(void);
void table_select(void);        /* boot overlay: call choose_table() */
void choose_table(void);
void leave_table(void);        /* main.c: leave, then the table list */
void test_start(uint8_t ph, uint8_t arg, uint8_t pev);
void test_poll(void);
void autoplay(void);

/* develop state the poll rate needs */
#define DV_WAIT   0
#define DV_PROMPT 1
#define DV_WALK   2
#define DV_OVER   3
extern uint8_t dv_state;

#endif
