/* trackinformation.c
 *      Copyright (C) 2011, Alexander Sulfrian
 *
 * This file is part of libnetmd.
 *
 * libnetmd is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Libnetmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <string.h>
#include <stdio.h>

#include "trackinformation.h"
#include "utils.h"
#include "log.h"

void netmd_get_track_information(netmd_dev_handle *dev, uint16_t track,
                                uint16_t p1, uint16_t p2,
                                unsigned char *data, size_t data_length)
{
    unsigned char cmd[] = { 0x00, 0x18, 0x06, 0x02, 0x20, 0x10, 0x01,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00,
                            0xff, 0x00, 0x00, 0x00, 0x00, 0x00 };
    unsigned char rsp[255];
    unsigned char *buf;
    int length;
    uint16_t real_data_length;
    size_t size;

    buf = cmd + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    netmd_copy_word_to_buffer(&buf, p1, 0);
    netmd_copy_word_to_buffer(&buf, p2, 0);

    length = netmd_exch_message(dev, cmd, sizeof(cmd), rsp);
    if (length > 0) {
        uint32_t tmp = (unsigned int)data[19] << 8;
        real_data_length = (tmp + data[20]) & 0xffffU;
        if (real_data_length > data_length) {
            size = data_length;
        }
        else {
            size = real_data_length;
        }

        memcpy(data, rsp + 21, size);
    }
}

int netmd_request_track_bitrate(netmd_dev_handle*dev, const uint16_t track,
                                unsigned char* encoding, unsigned char *channel)
{
    unsigned char info[8] = { 0 };

    netmd_get_track_information(dev, track, 0x3080, 0x0700, info, sizeof(info));
    memcpy(encoding, info + 6, 1);
    memcpy(channel, info + 7, 1);
    return 2;
}

int netmd_request_track_flags(netmd_dev_handle*dev, const uint16_t track, unsigned char* data)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x06, 0x01, 0x20, 0x10,
                               0x01, 0x00, 0x00, 0xff, 0x00, 0x00,
                               0x01, 0x00, 0x08};

    unsigned char *buf;
    unsigned char reply[255];

    buf = request + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    ret = netmd_exch_message(dev, request, sizeof(request), reply);
    *data = reply[ret - 1];
    return ret;
}

int netmd_request_title(netmd_dev_handle* dev, const uint16_t track, char* buffer, const size_t size)
{
    int ret = -1;
    size_t title_size = 0;
    unsigned char title_request[] = {0x00, 0x18, 0x06, 0x02, 0x20, 0x18,
                                     0x02, 0x00, 0x00, 0x30, 0x00, 0xa,
                                     0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
                                     0x00};
    unsigned char title[255];
    unsigned char *buf;

    buf = title_request + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    ret = netmd_exch_message(dev, title_request, 0x13, title);
    if(ret < 0)
    {
        fprintf(stderr, "bad ret code, returning early\n");
        return -1;
    }

    title_size = (size_t)ret;

    if(title_size == 0 || title_size == 0x13)
        return -1; /* bail early somethings wrong or no track */

    if(title_size > size)
    {
        printf("netmd_request_title: title too large for buffer\n");
        return -1;
    }

    memset(buffer, 0, size);
    memcpy(buffer, (title + 25), title_size - 25);
    buffer[size] = 0;

    return (int)title_size - 25;
}
