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

netmd_error
netmd_get_track_info(netmd_dev_handle *dev, netmd_track_index track_id, struct netmd_track_info *info)
{
    unsigned char flags, bitrate_id, channel;

    size_t title_len = netmd_request_title(dev, track_id, info->raw_title, sizeof(info->raw_title));

    if (title_len == -1) {
        return NETMD_TRACK_DOES_NOT_EXIST;
    }

    info->track_id = track_id;
    info->title = info->raw_title;

    if (!netmd_request_track_time(dev, track_id, &info->duration)) {
        return NETMD_ERROR;
    }

    if (!netmd_request_track_flags(dev, track_id, &flags)) {
        return NETMD_ERROR;
    }

    if (!netmd_request_track_encoding(dev, track_id, &bitrate_id, &channel)) {
        return NETMD_ERROR;
    }

    info->encoding = (enum NetMDEncoding)bitrate_id;
    info->channels = (enum NetMDChannels)channel;
    info->protection = (enum NetMDTrackFlags)flags;

    /*
     * Skip 'LP:' prefix, but only on tracks that are actually LP-encoded,
     * since a SP track might be titled beginning with "LP:" on purpose.
     * Note that since the MZ-R909 the "LP Stamp" can be disabled, so we
     * must check for the existence of the "LP:" prefix before skipping.
     */
    if ((info->encoding == NETMD_ENCODING_LP2 || info->encoding == NETMD_ENCODING_LP4) && strncmp(info->title, "LP:", 3) == 0) {
        info->title += 3;
    }

    return NETMD_NO_ERROR;
}

// Based on netmd-js / libnetmd.py
bool
netmd_request_track_flags(netmd_dev_handle *dev, netmd_track_index track, unsigned char *data)
{
    netmd_change_descriptor_state(dev, NETMD_DESCRIPTOR_AUDIO_CONTENTS_TD, NETMD_DESCRIPTOR_ACTION_OPEN_READ);

    struct netmd_bytebuffer *query = netmd_format_query("1806 01201001 %w ff00 00010008", track);
    struct netmd_bytebuffer *reply = netmd_send_query(dev, query);

    netmd_change_descriptor_state(dev, NETMD_DESCRIPTOR_AUDIO_CONTENTS_TD, NETMD_DESCRIPTOR_ACTION_CLOSE);

    return netmd_scan_query_buffer(reply, "1806 01201001 %?%? 10 00 00010008 %b", data);
}

int netmd_request_title(netmd_dev_handle* dev, netmd_track_index track, char* buffer, const size_t size)
{
    int ret = -1;
    size_t title_size = 0;
    unsigned char title_request[] = {0x00, 0x18, 0x06, 0x02, 0x20, 0x18,
                                     0x02, 0x00, 0x00, 0x30, 0x00, 0xa,
                                     0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
                                     0x00};
    char title[255];
    unsigned char *buf;

    buf = title_request + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    ret = netmd_exch_message(dev, title_request, 0x13, (unsigned char *)title);
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

// Based on https://github.com/cybercase/netmd-js/blob/master/src/netmd-interface.ts
static struct netmd_bytebuffer *
get_track_info(netmd_dev_handle *dev, netmd_track_index track, uint16_t p1, uint16_t p2)
{
    netmd_change_descriptor_state(dev, NETMD_DESCRIPTOR_AUDIO_CONTENTS_TD, NETMD_DESCRIPTOR_ACTION_OPEN_READ);

    struct netmd_bytebuffer *query = netmd_format_query("1806 02201001 %w %w %w ff00 00000000", track, p1, p2);
    struct netmd_bytebuffer *reply = netmd_send_query(dev, query);

    netmd_change_descriptor_state(dev, NETMD_DESCRIPTOR_AUDIO_CONTENTS_TD, NETMD_DESCRIPTOR_ACTION_CLOSE);

    struct netmd_bytebuffer *result = netmd_bytebuffer_new();

    if (!netmd_scan_query_buffer(reply, "1806 02201001 %?%? %?%? %?%? 1000 00%?0000 %x", &result)) {
        netmd_bytebuffer_free(result);
        return NULL;
    }

    return result;
}

bool
netmd_request_track_time(netmd_dev_handle* dev, netmd_track_index track, netmd_time *time)
{
    return netmd_scan_query_buffer(get_track_info(dev, track, 0x3000, 0x0100),
            "0001 0006 0000 %B %B %B %B",
            &time->hour, &time->minute, &time->second, &time->frame);
}

bool
netmd_request_track_encoding(netmd_dev_handle *dev, netmd_track_index track,
        unsigned char *encoding, unsigned char *channel)
{
    return netmd_scan_query_buffer(get_track_info(dev, track, 0x3080, 0x0700),
            "8007 0004 0110 %b %b", encoding, channel);
}
