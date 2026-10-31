#ifdef __H89__

/**
 * @brief   FujiNet calls - Heathkit H89 (CUSTOM_FUJINET_CALLS)
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 *
 * The shared code's network and AppKey calls, through fujinet-lib's h89 bus
 * (the H89 FujiNet card's 82C55 at 0xD0). They are routed here only so the
 * screen can be sent first: a call blocks for as long as the request takes,
 * and what was drawn just before it -- "refreshing game list..",
 * "connecting to server" -- has to be on the terminal meanwhile.
 */

#include <stdint.h>
#include <fujinet-fuji.h>
#include <fujinet-network.h>
#include "console.h"

int16_t custom_network_call(char *url, uint8_t *buffer, uint16_t max_len)
{
    static int16_t read;

    flush();
    if (network_open(url, OPEN_MODE_HTTP_GET, OPEN_TRANS_NONE) != FN_ERR_OK)
        return -1;

    read = network_read(url, buffer, max_len);
    network_close(url);
    return read;
}

uint16_t custom_read_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, char *destination)
{
    static uint16_t read;

    flush();
    read = 0;
    fuji_set_appkey_details(creator_id, app_id, DEFAULT);
    if (!fuji_read_appkey(key_id, &read, (uint8_t *)destination))
        read = 0;
    return read;
}

void custom_write_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, uint16_t count, char *data)
{
    flush();
    fuji_set_appkey_details(creator_id, app_id, DEFAULT);
    fuji_write_appkey(key_id, count, (uint8_t *)data);
}

#endif /* __H89__ */
