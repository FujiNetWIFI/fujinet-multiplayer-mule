/*
 * strings.c -- every string the game prints, in the Atari's wording
 * (clients/intv/text.bas, textlong.bas). A '%' is replaced by a number.
 *
 * The tables may live out of reach (the Apple II keeps them in auxiliary
 * RAM: this file's read-only data goes to the AUXTXT segment), so they are
 * only read through the str_*() functions, which hand back a copy. Two
 * copies are kept, so two strings can be in use at once.
 */
#include "mule.h"
#include "bank.h"

static const char *const titles[] = {
    "LAND GRANT", "LAND AUCTION #%", "DEVELOPMENT #%", "PRODUCTION #%", "TRANSPORT SHIP",
    "PEST ATTACK", "PIRATE SHIP", "ACID RAIN STORM", "PLANETQUAKE", "SUNSPOT ACTIVITY",
    "METEORITE", "RADIATION", "FIRE IN STORE", "TRANSPORT SHIP",
};

static const char *const msgs[] = {
    "",
    "YOU'RE LANDING ON THE PLANET IRATA.",
    "THE SHIP WILL BE BACK IN 12 MONTHS.",
    "PRESS YOUR BUTTON TO SELECT A PLOT.",
    "NO NEW PLOTS FOR SALE.",
    "PRESS YOUR STICK BUTTON TO START",
    "THE COMPUTER IS TAKING HIS TURN",
    "'S TURN",
    "TIME HAS RUN OUT",
    "WON $% GAMBLING!",
    "PRESS ALL PLAYER BUTTONS TO GO ON.",
    "FOOD SHORTAGE DECREASES TIME.",
    /* colony events 1..9 */
    "PESTS RUIN A PLOT'S FOOD CROP!",
    "PIRATES TAKE ALL CRYSTITE !",
    "FOOD OUTPUT UP, ENERGY REDUCED.",
    "MINING OUTPUT IS CUT IN HALF!",
    "ENERGY OUTPUT IS INCREASED",
    "A HUGE CRYSTITE DEPOSIT IS MADE",
    "MULE GOES CRAZY!",
    "ALL THE STOCK IN THE STORE IS LOST!",
    "THE SHIP IS BACK!",
    /* per-good status and auctions */
    "PREVIOUS AMOUNT",
    "USAGE",
    "SPOILAGE",
    "PRODUCTION",
    "STORE HAS % UNITS",
    "STORE HAS NO UNITS",
    "PUSH STICK TO DECLARE AS BUYER OR SELLER",
    "AUCTION IS OVER",
    "SELLER AT CRITICAL LEVEL!",
    "SELLER'S RUN DRY!",
    "STORE HAS RUN DRY!",
    "UNITS TRADED %",
    "PLOT IS SOLD!",
    "NO BIDS TENDERED",
    "BIDS START AT $%",
    "HIGH BID IS $%",
    "SALE PRICE: $%",
    "MONEY EQUALS $%",
    "NOT ENOUGH MONEY",
    "NO M.U.L.E.S LEFT",
    "ASSAY: NO CRYSTITE",
    "ASSAY: LOW CRYSTITE",
    "ASSAY: MEDIUM",
    "ASSAY: HIGH CRYSTITE",
    "ASSAY: VERY HIGH",
    "PLOT LISTED FOR SALE",
    "LISTING WITHDRAWN",
    "YOUR M.U.L.E. RAN OFF",
    "YOU CAUGHT THE WAMPUS!",
    "WATCHING GAME",
    "WAITING FOR PLAYERS",
    "READY",
    "COLLUSION",
    "PRESS BUTTON ON A PLOT TO SELL",
    "TAKE A SAMPLE FIRST",
    "OUTFIT M.U.L.E. $%",
    "THE SHIP IS BACK!",
    KEY_END_NAME " ENDS YOUR TURN",
    LOBBY_HINT,
    "STARTING IN % SECONDS",
};

static const char *const goods[] = { "FOOD", "ENERGY", "SMITHORE", "CRYSTITE" };

static const char *const species[] = {
    "MECHTRON", "GOLLUMER", "PACKER", "BONZOID", "SPHEROID", "FLAPPER", "LEGGITE", "HUMANOID",
};

static const char *const words[] = {
    "STATUS SUMMARY #", "MONEY", "LAND", "GOODS", "TOTAL", "COLONY", "FIRST FOUNDER",
    "AUCTION #", "STATUS #", "SELL", "BUY", "UNITS", "TRADED", "M.U.L.E.", "TOURNAMENT",
    "PRESS FOR", "***", "COMPUTER", "OPEN", "OVERALL, ",
};

/* Player events (turn.event 1..22), in the server's order
 * (internal/rules/events_player.go). '%' is the amount. */
static const char *const pevents[] = {
    "YOU JUST RECEIVED A PACKAGE FROM YOUR HOME-WORLD RELATIVES CONTAINING 3 FOOD AND 2 ENERGY UNITS.",
    "A WANDERING SPACE TRAVELER REPAID YOUR HOSPITALITY BY LEAVING TWO BARS OF SMITHORE.",
    "YOUR M.U.L.E. WAS JUDGED BEST BUILT AT THE COLONY FAIR. YOU WON $%.",
    "YOUR M.U.L.E. WON THE COLONY TAP-DANCING CONTEST. YOU COLLECTED $%.",
    "THE COLONY COUNCIL FOR AGRICULTURE AWARDED YOU $% FOR EACH FOOD PLOT YOU HAVE DEVELOPED. THE TOTAL GRANT IS $%.",
    "THE COLONY AWARDED YOU $% FOR STOPPING THE WART WORM INFESTATION.",
    "THE MUSEUM BOUGHT YOUR ANTIQUE PERSONAL COMPUTER FOR $%.",
    "YOU WON THE COLONY SWAMP EEL EATING CONTEST AND COLLECTED $%. (YECCH!)",
    "A CHARITY FROM YOUR HOME-WORLD TOOK PITY ON YOU AND SENT $%.",
    "YOUR OFFWORLD INVESTMENTS IN ARTIFICIAL DUMBNESS PAID $% IN DIVIDENDS.",
    "A DISTANT RELATIVE DIED AND LEFT YOU A VAST FORTUNE. BUT AFTER TAXES YOU ONLY GOT $%.",
    "YOU FOUND A DEAD MOOSE RAT AND SOLD THE HIDE FOR $%.",
    "YOU RECEIVED AN EXTRA PLOT OF LAND TO ENCOURAGE COLONY DEVELOPMENT.",
    "MISCHIEVOUS GLAC-ELVES BROKE INTO YOUR STORAGE SHED AND STOLE HALF YOUR FOOD.",
    "ONE OF YOUR M.U.L.E.S LOST A BOLT. REPAIRS COST YOU $%.",
    "YOUR MINING M.U.L.E.S HAVE DETERIORATED FROM HEAVY USE AND MUST BE REPAIRED. THE COST IS $%.",
    "THE SOLAR COLLECTORS ON YOUR ENERGY M.U.L.E.S ARE DIRTY. CLEANING COST YOU $%.",
    "YOUR SPACE GYPSY IN-LAWS MADE A MESS OF THE TOWN. IT COST YOU $% TO CLEAN IT UP.",
    "FLYING CAT-BUGS ATE THE ROOF OFF YOUR HOUSE. REPAIRS COST $%.",
    "YOU LOST $% BETTING ON THE TWO-LEGGED KAZINGA RACES.",
    "YOUR CHILD WAS BITTEN BY A BAT LIZARD AND THE HOSPITAL BILL COST YOU $%.",
    "YOU LOST A PLOT OF LAND BECAUSE THE CLAIM WAS NOT RECORDED.",
};

static const char *const ratings[] = {
    "THE COLONY FAILED. THE FEDERATION WILL NOT BE SENDING ANOTHER SHIP. YOU WILL ALL BE LEFT TO DIE.",
    "THE COLONY FAILED. THE FEW SURVIVORS ARE TAKEN OFF-PLANET BY FEDERATION POLICE. YOU HAVE BEEN SENTENCED TO PAY REPARATIONS.",
    "THE COLONY BARELY SURVIVED. THE FEDERATION HAS DECIDED TO KEEP IT RUNNING, BUT WITH A NEW SET OF LEADERS.",
    "THE COLONY WAS A SUCCESS. THE FEDERATION IS PLEASED AND WILL SEND NEW COLONISTS.",
    "THE COLONY WAS A GREAT SUCCESS. YOU WILL BE HONORED FOR YOUR WORK.",
    "THE COLONY WAS A TREMENDOUS SUCCESS. YOU WILL ALL BE NAMED HONORARY CITIZENS.",
    "THE COLONY WAS A PHENOMENAL SUCCESS! THE FEDERATION IS DELIGHTED AND HAS DECLARED IRATA A PLANETARY LANDMARK.",
};

static char sbuf[2][128];
static uint8_t sb;

static const char *far_str(const char *const *tab, uint8_t id)
{
    const char *p;
    char *b = sbuf[sb ^= 1];
    bank_get(&p, &tab[id], sizeof(p));
    bank_get(b, p, 127);
    b[127] = 0;
    return b;
}

const char *str_title(uint8_t id)   { return far_str(titles, id); }
const char *str_msg(uint8_t id)     { return far_str(msgs, id); }
const char *str_word(uint8_t id)    { return far_str(words, id); }
const char *str_good(uint8_t id)    { return far_str(goods, id & 3); }
const char *str_species(uint8_t id) { return far_str(species, id & 7); }
const char *str_pevent(uint8_t id)  { return far_str(pevents, id); }
const char *str_rating(uint8_t id)  { return id < 7 ? far_str(ratings, id) : ""; }
