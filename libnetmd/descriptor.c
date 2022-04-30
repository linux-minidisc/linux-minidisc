/*
 * Query formatting/scanning library for libnetmd
 * Based on netmd-js code by asivery (https://github.com/cybercase/netmd-js/pull/29)
 * Copyright (c) 2022, Thomas Perl <m@thp.io>
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


#include "descriptor.h"
#include "query.h"
#include "libnetmd.h"

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

// Taken from "export enum Descriptor" and "export enum DescriptorAction":
// https://github.com/cybercase/netmd-js/blob/master/src/netmd-interface.ts

// "TD" stands for "text database"

static const struct descriptor_to_format {
    enum netmd_descriptor descriptor;
    const char *format;
} DESCRIPTOR_FORMAT[] = {
    { NETMD_DESCRIPTOR_DISC_TITLE_TD,           "10 1801" },
    { NETMD_DESCRIPTOR_AUDIO_UTOC1_TD,          "10 1802" },
    { NETMD_DESCRIPTOR_AUDIO_UTOC4_TD,          "10 1803" },
    { NETMD_DESCRIPTOR_DSI_TD,                  "10 1804" },
    { NETMD_DESCRIPTOR_AUDIO_CONTENTS_TD,       "10 1001" },
    { NETMD_DESCRIPTOR_ROOT_TD,                 "10 1000" },

    { NETMD_DESCRIPTOR_DISC_SUBUNIT_IDENTIFIER, "00" },
    { NETMD_DESCRIPTOR_OPERATING_STATUS_BLOCK,  "80 00" },

    { NETMD_DESCRIPTOR_COUNT, NULL },
};

netmd_error
netmd_change_descriptor_state(netmd_dev_handle *dev,
        enum netmd_descriptor descriptor, enum netmd_descriptor_action action)
{
    const struct descriptor_to_format *cur = DESCRIPTOR_FORMAT;
    const char *format = NULL;

    while (cur->descriptor != NETMD_DESCRIPTOR_COUNT) {
        if (cur->descriptor == descriptor) {
            format = cur->format;
            break;
        }

        ++cur;
    }

    if (format == NULL) {
        return NETMD_ERROR;
    }

    char format_string[32];
    snprintf(format_string, sizeof(format_string), "1808 %s %02x 00", format, action);

    struct netmd_bytebuffer *query = netmd_format_query(format_string);
    if (query == NULL) {
        return NETMD_ERROR;
    }

    struct netmd_bytebuffer *reply = netmd_send_query(dev, query);
    if (reply == NULL) {
        return NETMD_ERROR;
    }

    netmd_bytebuffer_free(reply);
    return NETMD_NO_ERROR;
}

#ifdef __cplusplus
}
#endif
