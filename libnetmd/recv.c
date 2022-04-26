/*
 * recv.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2022 Thomas Perl <m@thp.io>
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

#include "libnetmd.h"


netmd_error netmd_recv_track(netmd_dev_handle *devh, int track_id, const char *filename)
{
    if (!netmd_dev_can_upload(devh)) {
        return NETMD_UNSUPPORTED_FEATURE;
    }

    char buf[256];

    if (filename == NULL) {
        if (netmd_request_title(devh, track_id, buf, sizeof(buf)) > sizeof(buf)) {
            // Title too long
            return NETMD_ERROR;
        }

        if (strlen(buf) == 0) {
            snprintf(buf, sizeof(buf), "Track %d", track_id);
        }
    } else {
        strncpy(buf, filename, sizeof(buf)-1);
    }

    uint8_t encoding;
    uint8_t channel;

    if (netmd_request_track_bitrate(devh, track_id, &encoding, &channel) != 2) {
        return NETMD_RESPONSE_NOT_EXPECTED;
    }

    if (encoding == NETMD_ENCODING_SP) {
        strncat(buf, ".aea", sizeof(buf)-1);
    } else if (encoding == NETMD_ENCODING_LP2 || encoding == NETMD_ENCODING_LP4) {
        strncat(buf, ".wav", sizeof(buf)-1);
    } else {
        return NETMD_TRACK_DOES_NOT_EXIST;
    }

    if (filename == NULL) {
        // Inform the user about the generated filename
        printf("%s\n", buf);
    }

    FILE *fp = fopen(buf, "wb");
    int result = netmd_secure_recv_track(devh, track_id, fp);
    fclose(fp);

    return result;
}
