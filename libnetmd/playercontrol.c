/*
 * playercontrol.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2022 Thomas Perl <m@thp.io>
 * Copyright (C) 2004 Bertrik Sikken
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

#include "playercontrol.h"
#include "utils.h"
#include "const.h"
#include "query.h"
#include "descriptor.h"
#include "libnetmd.h"


void netmd_time_double(netmd_time *time)
{
    int value = netmd_time_to_seconds(time);
    value *= 2;

    // TODO: This ignores the "frames"
    time->frame = 0;

    time->second = value % 60;
    value /= 60;

    time->minute = value % 60;
    value /= 60;

    time->hour = value;
}


int netmd_time_to_seconds(const netmd_time *time)
{
    return time->second + 60 * (time->minute + 60 * time->hour);
}

static netmd_error netmd_playback_control(netmd_dev_handle* dev, unsigned char code)
{
    unsigned char request[] = {0x00, 0x18, 0xc3, 0xff, 0x00, 0x00,
                               0x00, 0x00};
    unsigned char buf[255];
    int size;

    request[4] = code;
    size = netmd_exch_message(dev, request, sizeof(request), buf);

    if (size < 1) {
        return NETMD_ERROR;
    }

    return NETMD_NO_ERROR;
}

netmd_error netmd_play(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_PLAY);
}

netmd_error netmd_pause(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_PAUSE);
}

netmd_error netmd_fast_forward(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_FFORWARD);
}

netmd_error netmd_rewind(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_REWIND);
}

netmd_error netmd_stop(netmd_dev_handle* dev)
{
    unsigned char request[] = {0x00, 0x18, 0xc5, 0xff, 0x00, 0x00,
                               0x00, 0x00};
    unsigned char buf[255];
    int size;

    size = netmd_exch_message(dev, request, sizeof(request), buf);

    if(size < 0) {
        return NETMD_ERROR;
    }

    return NETMD_NO_ERROR;
}

netmd_error netmd_set_playmode(netmd_dev_handle* dev, const uint16_t playmode)
{
    unsigned char request[] = {0x00, 0x18, 0xd1, 0xff, 0x01, 0x00,
                               0x00, 0x00, 0x88, 0x08, 0x00, 0x00,
                               0x00};
    unsigned char buf[255];

    uint16_t tmp = playmode >> 8;
    request[10] = tmp & 0xffU;
    request[11] = playmode & 0xffU;

    /* TODO: error checkxing */
    netmd_exch_message(dev, request, sizeof(request), buf);

    return NETMD_NO_ERROR;
}

netmd_error netmd_set_current_track(netmd_dev_handle* dev, netmd_track_index track)
{
    unsigned char request[] = {0x00, 0x18, 0x50, 0xff, 0x01, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char buf[255];

    /* proper_to_bcd(track, request + 9, 2); */
    request[10] = track & 0xff;

    /* TODO: error checking */
    netmd_exch_message(dev, request, sizeof(request), buf);

    return NETMD_NO_ERROR;
}

netmd_error netmd_change_track(netmd_dev_handle* dev, const uint16_t direction)
{
    unsigned char request[] = {0x00, 0x18, 0x50, 0xff, 0x10, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char buf[255];

    uint16_t tmp = direction >> 8;
    request[9] = tmp & 0xffU;
    request[10] = direction & 0xffU;

    /* TODO: error checking */
    netmd_exch_message(dev, request, sizeof(request), buf);

    return NETMD_NO_ERROR;
}

netmd_error netmd_track_next(netmd_dev_handle* dev)
{
    return netmd_change_track(dev, NETMD_TRACK_NEXT);
}

netmd_error netmd_track_previous(netmd_dev_handle* dev)
{
    return netmd_change_track(dev, NETMD_TRACK_PREVIOUS);
}

netmd_error netmd_track_restart(netmd_dev_handle* dev)
{
    return netmd_change_track(dev, NETMD_TRACK_RESTART);
}

netmd_error netmd_set_playback_position(netmd_dev_handle* dev, netmd_track_index track, const netmd_time* time)
{
    unsigned char request[] = {0x00, 0x18, 0x50, 0xff, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00};
    unsigned char buf[255];

    proper_to_bcd(track, request + 9, 2);
    proper_to_bcd(time->hour, request + 11, 1);
    proper_to_bcd(time->minute, request + 12, 1);
    proper_to_bcd(time->second, request + 13, 1);
    proper_to_bcd(time->frame, request + 14, 1);

    /* TODO: error checking */
    netmd_exch_message(dev, request, sizeof(request), buf);

    return NETMD_NO_ERROR;
}

netmd_track_index
netmd_get_playback_position(netmd_dev_handle *dev, netmd_time *time)
{
    netmd_change_descriptor_state(dev, NETMD_DESCRIPTOR_OPERATING_STATUS_BLOCK, NETMD_DESCRIPTOR_ACTION_OPEN_READ);

    struct netmd_bytebuffer *query = netmd_format_query("1809 8001 0430 8802 0030 8805 0030 0003 0030 0002 00 ff00 00000000");
    struct netmd_bytebuffer *reply = netmd_send_query(dev, query);

    netmd_change_descriptor_state(dev, NETMD_DESCRIPTOR_OPERATING_STATUS_BLOCK, NETMD_DESCRIPTOR_ACTION_CLOSE);

    netmd_track_index track_id = NETMD_INVALID_TRACK;
    uint8_t hour = 0;

    if (netmd_scan_query(reply->data, reply->size,
                "1809 8001 0430 %?%? %?%? %?%? %?%? %?%? %?%? %?%? %? %?00 00%?0000 000b 0002 0007 00 %w %B %B %B %B",
                &track_id, &hour, &time->minute, &time->second, &time->frame)) {
        time->hour = hour;
    } else {
        track_id = NETMD_INVALID_TRACK;
    }

    netmd_bytebuffer_free(reply);

    return track_id;
}

netmd_error
netmd_get_disc_capacity(netmd_dev_handle *dev, netmd_disc_capacity *capacity)
{
    netmd_error result = NETMD_NO_ERROR;

    netmd_change_descriptor_state(dev, NETMD_DESCRIPTOR_ROOT_TD, NETMD_DESCRIPTOR_ACTION_OPEN_READ);

    struct netmd_bytebuffer *query = netmd_format_query("1806 02101000 3080 0300 ff00 00000000");
    struct netmd_bytebuffer *reply = netmd_send_query(dev, query);

    if (!netmd_scan_query(reply->data, reply->size,
                // 8003 changed to %?03 - Panasonic returns 0803 instead. This byte's meaning is unknown (asivery from netmd-js)
                //                                          vvvv
                "1806 02101000 3080 0300 1000 001d0000 001b %?03 0017 8000 0005 %W %B %B %B 0005 %W %B %B %B 0005 %W %B %B %B",
                &capacity->recorded.hour, &capacity->recorded.minute, &capacity->recorded.second, &capacity->recorded.frame,
                &capacity->total.hour, &capacity->total.minute, &capacity->total.second, &capacity->total.frame,
                &capacity->available.hour, &capacity->available.minute, &capacity->available.second, &capacity->available.frame)) {
        result = NETMD_ERROR;
    }

    netmd_change_descriptor_state(dev, NETMD_DESCRIPTOR_ROOT_TD, NETMD_DESCRIPTOR_ACTION_CLOSE);

    netmd_bytebuffer_free(reply);

    return result;
}
