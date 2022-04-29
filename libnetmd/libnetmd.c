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
 *
 * Portions based on netmd-js:
 * https://github.com/cybercase/netmd-js
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <unistd.h>

#include "libnetmd.h"
#include "utils.h"

#include <libusb.h>


// Taken from "export enum Descriptor" and "export enum DescriptorAction":
// https://github.com/cybercase/netmd-js/blob/master/src/netmd-interface.ts

// "TD" stands for "text database"

#if 0
static const char *
DESCRIPTOR_DISC_TITLE_TD = "10 1801";

static const char *
DESCRIPTOR_AUDIO_UTOC1_TD = "10 1802";

static const char *
DESCRIPTOR_AUDIO_UTOC4_TD = "10 1803";

static const char *
DESCRIPTOR_DSI_TD = "10 1804";
#endif

static const char *
DESCRIPTOR_AUDIO_CONTENTS_TD = "10 1001";

#if 0
static const char *
DESCRIPTOR_ROOT_TD = "10 1000";

static const char *
DESCRIPTOR_DISC_SUBUNIT_IDENTIFIER = "00";

static const char *
DESCRIPTOR_OPERATING_STATUS_BLOCK = "80 00"; // real name unknown
#endif

static const char *
DESCRIPTOR_ACTION_OPEN_READ = "01";

#if 0
static const char *
DESCRIPTOR_ACTION_OPEN_WRITE = "03";
#endif

static const char *
DESCRIPTOR_ACTION_CLOSE = "00";

static void
change_descriptor_state(netmd_dev_handle *dev, const char *descriptor, const char *action)
{
    char format_string[32];
    snprintf(format_string, sizeof(format_string), "1808 %s %s 00", descriptor, action);

    struct netmd_bytebuffer *query = netmd_format_query(format_string);
    struct netmd_bytebuffer *reply = netmd_send_query(dev, query);
    netmd_bytebuffer_free(reply);
}


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

struct netmd_bytebuffer *
netmd_send_query(netmd_dev_handle *dev, struct netmd_bytebuffer *query)
{
    uint8_t response[255];

    size_t request_len = query->size + 1;
    uint8_t *request = malloc(request_len);
    request[0] = NETMD_STATUS_CONTROL;
    memcpy(request + 1, query->data, query->size);

    int ret = netmd_exch_message(dev, request, request_len, response);
    free(request);

    if (ret > 0) {
        uint8_t response_code = response[0];
        if (response_code == NETMD_STATUS_NOT_IMPLEMENTED) {
            // Not implemented
        } else if (response_code == NETMD_STATUS_REJECTED) {
            // Rejected
        } else if (response_code != NETMD_STATUS_ACCEPTED &&
                   response_code != NETMD_STATUS_IMPLEMENTED &&
                   response_code != NETMD_STATUS_INTERIM) {
            // Unknown return status code
        } else {
            // Re-use the query object for the response, instead of free'ing
            // it and then re-allocating a new buffer for the reply
            query->size = 0;

            // Skip the first (status) byte when creating the reply
            return netmd_bytebuffer_append(query, (const char *)response + 1, ret - 1);
        }
    }

    netmd_bytebuffer_free(query);
    return NULL;
}

// Based on netmd-js / libnetmd.py
int
netmd_get_track_count(netmd_dev_handle *dev)
{
    int result = -1;

    change_descriptor_state(dev, DESCRIPTOR_AUDIO_CONTENTS_TD, DESCRIPTOR_ACTION_OPEN_READ);

    struct netmd_bytebuffer *query = netmd_format_query("1806 02101001 3000 1000 ff00 00000000");
    struct netmd_bytebuffer *reply = netmd_send_query(dev, query);

    if (reply == NULL) {
        return -1;
    }

    uint8_t num_tracks = 0;
    if (netmd_scan_query(reply->data, reply->size, "1806 02101001 %?%? %?%? 1000 00%?0000 0006 0010000200%b", &num_tracks)) {
        result = num_tracks;
    }

    netmd_bytebuffer_free(reply);

    change_descriptor_state(dev, DESCRIPTOR_AUDIO_CONTENTS_TD, DESCRIPTOR_ACTION_CLOSE);

    return result;
}
