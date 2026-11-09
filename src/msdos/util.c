#ifdef __WATCOMC__

#include <i86.h>
#include <dos.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <direct.h>
#include <fujinet-fuji.h>
#include "../misc.h"
#include "../platform-specific/graphics.h"

extern unsigned char prevVideoMode;

static clock_t startClock;

#define FUJI_SIGNATURE "FUJI"

typedef struct {
    uint8_t query;
    char    signature[4];
    uint8_t unit;
} fuji_ioctl_query;

// The DOS drive (1 = A:) the FujiNet driver maps to device slot `slot`, or -1
static int find_slot_drive(int slot)
{
    int drive;
    union REGS regs;
    struct SREGS sregs;
    fuji_ioctl_query query;

    for (drive = 3; drive <= 26; drive++) {
        memset(&query, 0, sizeof(query));
        regs.h.ah = 0x44;
        regs.h.al = 0x04;
        regs.h.bl = (unsigned char)drive;
        regs.w.cx = sizeof(query);
        regs.x.dx = FP_OFF(&query);
        sregs.ds  = FP_SEG(&query);
        int86x(0x21, &regs, &regs, &sregs);
        if (!(regs.x.cflag & INTR_CF) &&
            memcmp(query.signature, FUJI_SIGNATURE, 4) == 0 &&
            query.unit == slot)
            return drive;
    }
    return -1;
}

void resetTimer()
{
    startClock = clock();
}

uint16_t getTime()
{
    return (uint16_t)((clock() - startClock) * 60 / CLOCKS_PER_SEC);
}

void quit()
{
    union REGS r;
    unsigned int total;
    int drive;

    r.h.ah = 0x00;
    r.h.al = prevVideoMode;
    int86(0x10, &r, &r);

    printf("Loading Lobby...\n");
    fuji_set_boot_mode(FUJI_BOOT_MODE_LOBBY);

    // Disk reset, so DOS rereads the drive the lobby was mounted on
    r.h.ah = 0x0D;
    int86(0x21, &r, &r);

    drive = find_slot_drive(0);
    if (drive < 0)
        exit(0);

    _dos_setdrive(drive, &total);
    chdir("\\");
    spawnlp(P_OVERLAY, "COMMAND.COM", "COMMAND.COM", "/C", "AUTOEXEC.BAT", NULL);
    exit(0);
}

void housekeeping()
{
}

uint8_t getJiffiesPerSecond()
{
    return 60;
}

#endif /* __WATCOMC__ */
