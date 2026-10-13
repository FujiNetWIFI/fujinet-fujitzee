#ifdef BUILD_ATARI5200

/**
 * @brief   FujiNet calls - Atari 5200 (CUSTOM_FUJINET_CALLS, and the lobby boot)
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * Everything that talks to the cartridge through fujinet-lib is in this one
 * file, as on the 7800 (src/atari7800/fujinet.c): it includes the library's
 * headers and, of the game's, only vars.h; the shared code reaches the
 * cartridge through the CUSTOM_FUJINET_CALLS hooks instead.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fujinet-fuji.h>
#include <fujinet-network.h>
#include <fujinet-atari5200.h>
#include <atari5200.h>
#include "vars.h"

// graphics.c - declared here rather than through ../platform-specific, which
// would bring in the game's headers
void drawText(uint8_t x, uint8_t y, const char *s);
void drawSpace(uint8_t x, uint8_t y, uint8_t w);

// READ HOST SLOTS returns all 8 slots.
#define FUJI_HOST_SLOT_COUNT 8
#define LOBBY_DEVICE_SLOT 0
#define LOBBY_MODE_READ 1

int16_t custom_network_call(char *url, uint8_t *buffer, uint16_t max_len)
{
    int16_t read;

    if (network_open(url, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE) != FN_ERR_OK)
        return -1;

    read = network_read(url, buffer, max_len);
    network_close(url);
    return read;
}

uint16_t custom_read_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, char *destination)
{
    uint16_t read = 0;

    fuji_set_appkey_details(creator_id, app_id, DEFAULT);
    if (!fuji_read_appkey(key_id, &read, (uint8_t *)destination))
        read = 0;
    return read;
}

void custom_write_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, uint16_t count, char *data)
{
    fuji_set_appkey_details(creator_id, app_id, DEFAULT);
    fuji_write_appkey(key_id, count, (uint8_t *)data);
}

// Lower case: the charset's upper case codes are board art
static void quitStatus(const char *s)
{
    drawSpace(3, HEIGHT - 1, WIDTH - 6);
    drawText((WIDTH - (unsigned char)strlen(s)) / 2, HEIGHT - 1, s);
}

static bool sameHost(const char *a, const char *b)
{
    while (*a && *b)
    {
        if ((*a | 0x20) != (*b | 0x20))
            return false;
        a++;
        b++;
    }
    return *a == *b;
}

/*
  SET_DEVICE_FULLPATH is a fixed 256-byte payload -- a short one is rejected on
  the ESP32 side -- so the path is padded here rather than at the call site.
  The path is CONFIG's PLAY GAME LOBBY one.
*/
static const char lobbyPath[MAX_FILENAME_LEN] = "/atari5200/lobby.a52";
static const char lobbyHost[] = "ec.tnfs.io";
static HostSlot slots[FUJI_HOST_SLOT_COUNT];

static uint8_t findLobbyHost(void)
{
    uint8_t i;

    if (!fuji_get_host_slots(slots, FUJI_HOST_SLOT_COUNT))
        return FUJI_HOST_SLOT_COUNT;

    for (i = 0; i < FUJI_HOST_SLOT_COUNT; i++)
        if (sameHost(lobbyHost, (const char *)slots[i]))
            return i;

    return FUJI_HOST_SLOT_COUNT;
}

/*
  The slot is used as found, never created: a player who reached this client
  through the FujiNet Lobby already has the host, and anyone else is told to
  add it in CONFIG.
*/
static bool mountLobby(uint8_t slot)
{
    if (!fuji_mount_host_slot(slot))
        return false;
    if (!fuji_set_device_filename(LOBBY_MODE_READ, slot, LOBBY_DEVICE_SLOT,
                                  (char *)lobbyPath))
        return false;
    // The image can reach the cartridge after this has been answered.
    return fuji_mount_disk_image(LOBBY_DEVICE_SLOT, LOBBY_MODE_READ);
}

static bool loadLobby(void)
{
    uint8_t bootState, slot, pct = 0xFF;
    char pctText[4];

    slot = findLobbyHost();
    if (slot == FUJI_HOST_SLOT_COUNT)
    {
        quitStatus("add ec.tnfs.io in config");
        return false;
    }

    if (!mountLobby(slot))
    {
        quitStatus("lobby not available");
        return false;
    }

    // Watch the cartridge's own progress counter rather than guessing a delay.
    for (;;)
    {
        bootState = fuji_a5200_boot_state();
        if (bootState == FUJI_A5200_BOOT_READY)
            return true;
        if (bootState == FUJI_A5200_BOOT_FAILED)
        {
            quitStatus("lobby load failed");
            return false;
        }
        if (fuji_a5200_boot_percent() != pct)
        {
            pct = fuji_a5200_boot_percent();
            itoa(pct, pctText, 10);
            drawText(WIDTH / 2 + 9, HEIGHT - 1, pctText);
        }
    }
}

// To the lobby, as on the 7800; failing that, back to CONFIG, which is
// where the 8-bit client's quit goes.
void quit(void)
{
    uint8_t i;

    quitStatus("loading lobby...");

    if (!fuji_a5200_present())
    {
        quitStatus("no fujinet cartridge");
        return;
    }

    if (loadLobby())
        fuji_a5200_boot();

    // Long enough to read why
    for (i = 0; i < 120; i++)
        waitvsync();
    quitStatus("returning to config...");
    fuji_a5200_config();
}

#endif /* BUILD_ATARI5200 */
