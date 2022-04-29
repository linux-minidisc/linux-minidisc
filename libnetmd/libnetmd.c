/*
 * libnetmd.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2022 Thomas Perl <m@thp.io>
 * Copyright (C) 2002, 2003 Marc Britten
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

#include <unistd.h>

#include "libnetmd.h"
#include "utils.h"

#include <libusb.h>

const char *
netmd_get_encoding_name(enum NetMDEncoding encoding, enum NetMDChannels channels)
{
    switch (channels) {
        case NETMD_CHANNELS_MONO:
            return "Mono";
        case NETMD_CHANNELS_STEREO:
            switch (encoding) {
                case NETMD_ENCODING_SP:
                    return "SP";
                case NETMD_ENCODING_LP2:
                    return "LP2";
                case NETMD_ENCODING_LP4:
                    return "LP4";
                default:
                    break;
            }
            break;
        default:
            break;
    }

    return "UNKNOWN";
}

const char *
netmd_track_flags_to_string(enum NetMDTrackFlags flags)
{
    switch (flags) {
        case NETMD_TRACK_FLAG_UNPROTECTED:
            return "UnPROT";
        case NETMD_TRACK_FLAG_PROTECTED:
            return "TrPROT";
        default:
            return "UNKNOWN";
    }
}

ssize_t
netmd_get_disc_title(netmd_dev_handle* dev, char* buffer, size_t size)
{
    int ret = -1;
    size_t title_size = 0;
    unsigned char title_request[] = {0x00, 0x18, 0x06, 0x02, 0x20, 0x18,
                                     0x01, 0x00, 0x00, 0x30, 0x00, 0xa,
                                     0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
                                     0x00};
    unsigned char title[255];

    ret = netmd_exch_message(dev, title_request, 0x13, title);
    if(ret < 0)
    {
        netmd_log(NETMD_LOG_WARNING, "netmd_get_disc_title(): bad ret code, returning early\n");
        return 0;
    }

    title_size = (size_t)ret;

    if(title_size == 0 || title_size == 0x13)
        return -1; /* bail early somethings wrong */

    if((title_size - 25) >= size)
    {
        netmd_log(NETMD_LOG_WARNING, "netmd_get_disc_title(): title too large for buffer\n");
    }
    else
    {
        memset(buffer, 0, size);
        memcpy(buffer, (title + 25), title_size - 25);
        buffer[title_size - 25] = 0;
    }

    netmd_log(NETMD_LOG_VERBOSE, "netmd_get_disc_title() -> \"%s\"\n", buffer);

    return title_size - 25;
}

int netmd_set_track_title(netmd_dev_handle* dev, netmd_track_index track, const char *buffer)
{
    int ret = 1;
    unsigned char *title_request = NULL;
    /* handshakes for 780/980/etc */
    unsigned char hs2[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x00, 0x00};
    unsigned char hs3[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x03, 0x00};

    unsigned char title_header[] = {0x00, 0x18, 0x07, 0x02, 0x20, 0x18,
                                    0x02, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                    0x00, 0x50, 0x00, 0x00, 0x0a, 0x00,
                                    0x00, 0x00, 0x0d};
    unsigned char reply[255];
    unsigned char *buf;
    size_t size;
    int oldsize;

    /* the title update command wants to now how many bytes to replace */
    oldsize = netmd_request_title(dev, track, (char *)reply, sizeof(reply));
    if(oldsize == -1)
        oldsize = 0; /* Reading failed -> no title at all, replace 0 bytes */

    size = strlen(buffer);
    title_request = malloc(sizeof(char) * (0x15 + size));
    memcpy(title_request, title_header, 0x15);
    memcpy((title_request + 0x15), buffer, size);

    buf = title_request + 7;
    netmd_copy_word_to_buffer(&buf, track, 0);
    title_request[16] = size & 0xff;
    title_request[20] = oldsize & 0xff;


    /* send handshakes */
    netmd_exch_message(dev, hs2, 8, reply);
    netmd_exch_message(dev, hs3, 8, reply);

    ret = netmd_exch_message(dev, title_request, 0x15 + size, reply);
    free(title_request);

    if(ret < 0)
    {
        netmd_log(NETMD_LOG_WARNING, "netmd_set_title: exchange failed, ret=%d\n", ret);
        return 0;
    }

    return 1;
}

int netmd_move_track(netmd_dev_handle* dev, const uint16_t start, const uint16_t finish)
{
    int ret = 0;
    unsigned char hs[] = {0x00, 0x18, 0x08, 0x10, 0x10, 0x01, 0x00, 0x00};
    unsigned char request[] = {0x00, 0x18, 0x43, 0xff, 0x00, 0x00,
                               0x20, 0x10, 0x01, 0x00, 0x04, 0x20,
                               0x10, 0x01, 0x00, 0x03};
    unsigned char reply[255];
    unsigned char *buf;

    // TODO: Need to update groups

    buf = request + 9;
    netmd_copy_word_to_buffer(&buf, start, 0);

    buf = request + 14;
    netmd_copy_word_to_buffer(&buf, finish, 0);

    netmd_exch_message(dev, hs, 8, reply);
    netmd_exch_message(dev, request, 16, reply);
    ret = netmd_exch_message(dev, request, 16, reply);

    if(ret < 0)
    {
        fprintf(stderr, "bad ret code, returning early\n");
        return 0;
    }

    return 1;
}

int netmd_set_disc_title(netmd_dev_handle* dev, const char* title)
{
    unsigned char *request, *p;
    unsigned char write_req[] = {0x00, 0x18, 0x07, 0x02, 0x20, 0x18,
                                 0x01, 0x00, 0x00, 0x30, 0x00, 0x0a,
                                 0x00, 0x50, 0x00, 0x00};
    unsigned char hs1[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x01, 0x00};
    unsigned char hs2[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x00, 0x00};
    unsigned char hs3[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x03, 0x00};
    unsigned char hs4[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x01, 0x00, 0x00};
    unsigned char reply[256];
    int result;

    /* the title update command wants to now how many bytes to replace */
    ssize_t oldsize = netmd_get_disc_title(dev, (char *)reply, sizeof(reply));
    if (oldsize == -1) {
        oldsize = 0; /* Reading failed -> no title at all, replace 0 bytes */
    }

    netmd_log(NETMD_LOG_VERBOSE, "netmd_set_disc_title(title=\"%s\")\n", title);

    size_t title_length = strlen(title);

    request = malloc(21 + title_length);
    memset(request, 0, 21 + title_length);

    memcpy(request, write_req, 16);
    request[16] = title_length & 0xff;
    request[20] = oldsize & 0xff;

    p = request + 21;
    memcpy(p, title, title_length);

    /* send handshakes */
    netmd_exch_message(dev, hs1, sizeof(hs1), reply);
    netmd_exch_message(dev, hs2, sizeof(hs2), reply);
    netmd_exch_message(dev, hs3, sizeof(hs3), reply);
    result = netmd_exch_message(dev, request, 0x15 + title_length, reply);
    /* send handshake to write */
    netmd_exch_message(dev, hs4, sizeof(hs4), reply);
    return result;
}

/* AV/C Disc Subunit Specification ERASE (0x40),
 * subfunction "specific_object" (0x01) */
int netmd_delete_track(netmd_dev_handle* dev, netmd_track_index track)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x40, 0xff, 0x01, 0x00,
                               0x20, 0x10, 0x01, 0x00, 0x00};
    unsigned char reply[255];
    unsigned char *buf;

    // TODO: Need to update groups

    buf = request + 9;
    netmd_copy_word_to_buffer(&buf, track, 0);
    ret = netmd_exch_message(dev, request, 11, reply);

    return ret;
}

/* AV/C Disc Subunit Specification ERASE (0x40),
 * subfunction "complete" (0x00) */
int netmd_erase_disc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x40, 0xff, 0x00, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, 11, reply);

    return ret;
}

/* AV/C Description Spefication OPEN DESCRIPTOR (0x08),
 * subfunction "open for write" (0x03) */
int netmd_cache_toc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x03, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, sizeof(request), reply);
    return ret;
}

/* AV/C Description Spefication OPEN DESCRIPTOR (0x08),
 * subfunction "close" (0x00) */
int netmd_sync_toc(netmd_dev_handle* dev)
{
    int ret = 0;
    unsigned char request[] = {0x00, 0x18, 0x08, 0x10, 0x18, 0x02, 0x00, 0x00};
    unsigned char reply[255];

    ret = netmd_exch_message(dev, request, sizeof(request), reply);
    return ret;
}

/* Calls need for Sharp devices */
int netmd_acquire_dev(netmd_dev_handle* dev)
{
    // Unit command: "subunit_type = 0x1F, subunit_id = 7 -> 0xFF"
    // AVC Digital Interface Commands 3.0, 9. Unit commands"
    unsigned char request[] = {0x00, 0xff, 0x01, 0x0c, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    unsigned char reply[255];

    // TODO: Check return value
    netmd_exch_message(dev, request, sizeof(request), reply);
    if (reply[0] == NETMD_STATUS_ACCEPTED){
      return NETMD_NO_ERROR;
    } else {
      return NETMD_COMMAND_FAILED_UNKNOWN_ERROR;
    }
}

int netmd_release_dev(netmd_dev_handle* dev)
{
    // Unit command: "subunit_type = 0x1F, subunit_id = 7 -> 0xFF"
    // AVC Digital Interface Commands 3.0, 9. Unit commands"
    unsigned char request[] = {0x00, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    unsigned char reply[255];

    return netmd_exch_message(dev, request, sizeof(request), reply);
}

const char *
netmd_minidisc_get_disc_name(const minidisc *md)
{
    return md->groups[0].name;
}

netmd_group_id
netmd_minidisc_get_track_group(const minidisc *md, netmd_track_index track_id)
{
    // Tracks in the groups are 1-based
    track_id += 1;

    for (int group = 1; group < md->group_count; group++) {
        if ((md->groups[group].first <= track_id) && (md->groups[group].last >= track_id)) {
            return group;
        }
    }

    return 0;
}

const char *
netmd_minidisc_get_group_name(const minidisc *md, netmd_group_id group)
{
    if (group == 0 || group >= md->group_count) {
        return NULL;
    }

    return md->groups[group].name;
}

bool
netmd_minidisc_is_group_empty(const minidisc *md, netmd_group_id group)
{
    // Non-existent groups are always considered "empty"
    if (group == 0 || group >= md->group_count) {
        return true;
    }

    return (md->groups[group].first == 0 && md->groups[group].last == 0);
}

int
netmd_get_track_count(netmd_dev_handle *dev)
{
    // See libnetmd.py, getTrackCount()
    // TODO: Figure out the corresponding AV/C structures
    uint8_t request[] = {
        0x00, 0x18, 0x06,
        0x02, 0x10, 0x10, 0x01,
        0x30, 0x00,
        0x10, 0x00,
        0xff, 0x00,
        0x00, 0x00, 0x00, 0x00,
    };

    uint8_t response[255];

    static const uint8_t expected_payload_length = 6;
    static const uint8_t expected_payload_prefix[] = {
        0x00, 0x10, 0x00, 0x02, 0x00,
    };

    int ret = netmd_exch_message(dev, request, sizeof(request), response);
    if (ret != sizeof(request) + sizeof(uint16_t) + expected_payload_length) {
        netmd_log(NETMD_LOG_WARNING, "Could not retrieve number of tracks\n");
        return -1;
    }

    // Response data begins after query data
    const uint8_t *resp = response + sizeof(request);

    uint16_t payload_length = netmd_convert_word(*((const uint16_t *)resp));
    if (payload_length != expected_payload_length) {
        return -1;
    }

    const uint8_t *payload = resp + sizeof(uint16_t);

    if (memcmp(payload, expected_payload_prefix, sizeof(expected_payload_prefix)) == 0) {
        return payload[payload_length-1];
    }

    return -1;
}
