/*
 * trackinformation.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2011 Alexander Sulfrian
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "trackinformation.h"
#include "utils.h"
#include "log.h"
#include "libnetmd.h"

#include <stdlib.h>

void netmd_get_track_information(netmd_dev_handle *dev, netmd_track_index track,
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

int netmd_request_track_bitrate(netmd_dev_handle*dev, netmd_track_index track,
                                unsigned char* encoding, unsigned char *channel)
{
    unsigned char cmd[] = { 0x00, 0x18, 0x06, 0x02, 0x20, 0x10, 0x01,  0x00, 0x00,  0x30, 0x80,  0x07, 0x00,
                            0xff, 0x00, 0x00, 0x00, 0x00, 0x00 };
    unsigned char rsp[255];
    unsigned char *buf;
    unsigned char info[8] = { 0 };
    unsigned char flags;
    struct netmd_track time;

    msleep(5); // Sleep fixes 'unknown' bitrate being returned on many devices.

    // Copy the track number into the request
    buf = cmd + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    //send request to device
    int length = netmd_exch_message(dev, cmd, sizeof(cmd), rsp);

    // pull encoding and channel from response
    if (length >= 29) {
      memcpy(encoding, rsp + 27, 1);
      memcpy(channel, rsp + 28, 1);
    } else {
      memset(encoding, 0, 1);
      memset(channel, 0, 1);
    }

    return 2;
}

int netmd_request_track_flags(netmd_dev_handle*dev, netmd_track_index track, unsigned char* data)
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

int netmd_request_title(netmd_dev_handle* dev, netmd_track_index track, char* buffer, const size_t size)
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

    int title_response_header_size = 25;
    const char *title_text = title + title_response_header_size;
    size_t required_size = title_size - title_response_header_size;

    if (required_size > size - 1)
    {
        printf("netmd_request_title: title too large for buffer\n");
        return -1;
    }

    memset(buffer, 0, size);
    memcpy(buffer, title_text, required_size);

    return required_size;
}

int netmd_request_track_time(netmd_dev_handle* dev, netmd_track_index track, struct netmd_track* buffer)
{
    int ret = 0;
    unsigned char hs[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x01, 0x00};
    unsigned char request[] = {0x00, 0x18, 0x06, 0x02, 0x20, 0x10,
                               0x01, 0x00, 0x01, 0x30, 0x00, 0x01,
                               0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
                               0x00};
    unsigned char time_request[255];
    unsigned char *buf;

    buf = request + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    netmd_exch_message(dev, hs, 8, time_request);
    ret = netmd_exch_message(dev, request, 0x13, time_request);
    if(ret < 0)
    {
        fprintf(stderr, "bad ret code, returning early\n");
        return 0;
    }

    // TODO: Could add "hour" to netmd_track struct later, but all consumers need to be updated then
    int hours = bcd_to_proper(time_request + 27, 1) & 0xff;

    buffer->minute = (bcd_to_proper(time_request + 28, 1) & 0xff) + hours * 60;
    buffer->second = bcd_to_proper(time_request + 29, 1) & 0xff;
    buffer->tenth = bcd_to_proper(time_request + 30, 1) & 0xff;

    return 1;
}
