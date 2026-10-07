/*
 * lobby.c -- back to the FujiNet lobby, as the FujiNet's other DOS games
 * go (fujinet-5cardstud src/msdos/util.c): mount the lobby's disk from its
 * TNFS host in device slot 0, find the DOS drive FUJINET.SYS gives that
 * slot, and run its AUTOEXEC.BAT.
 *
 * The other games spawn COMMAND.COM for that, which brings 11K of the C
 * library along. Here the lobby's drive is made DOS's current one and
 * "AUTOEXEC" + ENTER is put in the BIOS's keyboard buffer: the game exits,
 * all its memory is free, and COMMAND.COM reads the line as if typed.
 */
#include <string.h>
#include <dos.h>
#include <i86.h>
#include "fujinet-fuji.h"

#define LOBBY_HOST "ec.tnfs.io"
#define LOBBY_FILE "msdos/lobby.img"
#define HOST_SLOTS   8
#define DEVICE_SLOTS 8

static HostSlot host_slots[HOST_SLOTS];
static DeviceSlot device_slots[DEVICE_SLOTS];
static char fname[MAX_FILENAME_LEN];

/* What FUJINET.SYS answers an IOCTL read (INT 21h AX=4404h) with. */
typedef struct {
    uint8_t query;
    char signature[4];          /* "FUJI" */
    uint8_t unit;               /* the device slot */
} FujiQuery;

static int slot_drive(uint8_t slot)
{
    union REGS r;
    struct SREGS s;
    FujiQuery q;
    int drive;

    for (drive = 3; drive <= 26; ++drive) {
        memset(&q, 0, sizeof(q));
        r.w.ax = 0x4404;
        r.h.bl = (uint8_t)drive;
        r.w.cx = sizeof(q);
        r.w.dx = FP_OFF(&q);
        segread(&s);
        s.ds = FP_SEG(&q);
        intdosx(&r, &r, &s);
        if (!r.w.cflag && memcmp(q.signature, "FUJI", 4) == 0 && q.unit == slot)
            return drive;
    }
    return -1;
}

/* Put keys in the BIOS's keyboard buffer (0040:001A head, 001C tail,
 * 001E..003D the buffer), as if typed. (plat.c too: MULEJR.EXE leaves a
 * machine it can't run on to MULE.EXE.) */
void type_in(const char *k)
{
    uint16_t __far *head = MK_FP(0x40, 0x1A);
    uint16_t __far *tail = MK_FP(0x40, 0x1C);
    uint16_t t, n;
    _disable();
    *head = *tail = 0x1E;
    for (t = 0x1E; *k; ++k) {
        n = t + 2 >= 0x3E ? 0x1E : t + 2;
        if (n == *head)
            break;
        *(uint16_t __far *)MK_FP(0x40, t) = (uint8_t)*k | (*k == '\r' ? 0x1C00 : 0);
        t = n;
    }
    *tail = t;
    _enable();
}

static const char root[] = "\\";

/* Mounts the lobby and leaves it to be run when the game exits. */
void lobby_return(void)
{
    union REGS r;
    struct SREGS s;
    uint8_t i, slot = HOST_SLOTS;
    int drive;

    extern uint8_t plat_net_probe(void);    /* plat.c */
    if (!plat_net_probe())
        return;                 /* no FUJINET.SYS */
    if (!fuji_get_host_slots(host_slots, HOST_SLOTS))
        return;
    for (i = 0; i < HOST_SLOTS; ++i)
        if (strcmp((char *)host_slots[i], LOBBY_HOST) == 0) {
            slot = i;
            break;
        }
    if (slot == HOST_SLOTS) {
        slot = HOST_SLOTS - 1;
        strcpy((char *)host_slots[slot], LOBBY_HOST);
        fuji_put_host_slots(host_slots, HOST_SLOTS);
    }
    fuji_mount_host_slot(slot);
    fuji_get_device_slots(device_slots, DEVICE_SLOTS);
    device_slots[0].hostSlot = slot;
    device_slots[0].mode = 0;
    strcpy((char *)device_slots[0].file, LOBBY_FILE);
    fuji_put_device_slots(device_slots, DEVICE_SLOTS);
    strcpy(fname, LOBBY_FILE);
    fuji_set_device_filename(0, slot, 0, fname);
    fuji_mount_disk_image(0, 1);

    drive = slot_drive(0);
    if (drive < 0)
        return;
    r.h.ah = 0x0E;              /* the current drive (0 = A:) */
    r.h.dl = (uint8_t)(drive - 1);
    intdos(&r, &r);
    r.h.ah = 0x3B;              /* its root */
    r.w.dx = FP_OFF(root);
    segread(&s);
    intdosx(&r, &r, &s);
    type_in("AUTOEXEC\r");
}
