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

int netmd_request_track_bitrate(netmd_dev_handle*dev, netmd_track_index track,
                                unsigned char* encoding, unsigned char *channel)
{
    unsigned char cmd[] = { 0x00, 0x18, 0x06, 0x02, 0x20, 0x10, 0x01,  0x00, 0x00,  0x30, 0x80,  0x07, 0x00,
                            0xff, 0x00, 0x00, 0x00, 0x00, 0x00 };
    unsigned char rsp[255];
    unsigned char *buf;

    msleep(5); // Sleep fixes 'unknown' bitrate being returned on many devices.

    // Copy the track number into the request
    buf = cmd + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    //send request to device
    int length = netmd_exch_message(dev, cmd, sizeof(cmd), rsp);

    // AV/C MD Audio 1.0, Table 7-3 audio_recording_parameters_info_block
    // AV/C Disc Subunit 1.0, Table 11.7 (page 134) "Audio Recording Parameters Info Block"
    struct audio_recording_parameters_info_block {
        uint16_t compound_length; // 8 (big endian)
        uint16_t info_block_type; // 0x8007 (audio_recording_parameters_info_block, big endian)
        uint16_t primary_fields_length; // 4 (big endian)
        uint8_t audio_recording_sample_rate; // 0x01 = 44100 Hz
        uint8_t audio_recording_sample_size; // 0x10 = 16-bit
        uint8_t audio_compression_mode; // 0x90 = ATRAC1
        uint8_t audio_recording_channel_mode; // 0x00 = stereo, 0x01 = mono
    };

    // pull encoding and channel from response
    if (length >= sizeof(cmd) + sizeof(struct audio_recording_parameters_info_block)) {
        const struct audio_recording_parameters_info_block *info = (void *)(rsp + sizeof(cmd));

        netmd_log(NETMD_LOG_DEBUG, "compound_length: 0x%04x\n", netmd_convert_word(info->compound_length));
        netmd_log(NETMD_LOG_DEBUG, "info_block_type: 0x%04x\n", netmd_convert_word(info->info_block_type));
        netmd_log(NETMD_LOG_DEBUG, "primary_fields_length: 0x%04x\n", netmd_convert_word(info->primary_fields_length));
        netmd_log(NETMD_LOG_DEBUG, "audio_recording_sample_rate: 0x%02x\n", info->audio_recording_sample_rate);
        netmd_log(NETMD_LOG_DEBUG, "audio_recording_sample_size: 0x%02x\n", info->audio_recording_sample_size);
        netmd_log(NETMD_LOG_DEBUG, "audio_compression_mode: 0x%02x\n", info->audio_compression_mode);
        netmd_log(NETMD_LOG_DEBUG, "audio_recording_channel_mode: 0x%02x\n", info->audio_recording_channel_mode);

        *encoding = info->audio_compression_mode;
        *channel = info->audio_recording_channel_mode;
    } else {
        *encoding = 0;
        *channel = 0;
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

int netmd_request_track_time(netmd_dev_handle* dev, netmd_track_index track, netmd_time *time)
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

    time->hour = bcd_to_proper(time_request + 27, 1) & 0xff;
    time->minute = (bcd_to_proper(time_request + 28, 1) & 0xff);
    time->second = bcd_to_proper(time_request + 29, 1) & 0xff;
    time->frame = bcd_to_proper(time_request + 30, 1) & 0xff;

    return 1;
}
