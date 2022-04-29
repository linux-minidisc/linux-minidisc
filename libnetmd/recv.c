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

const char *
netmd_recv_get_file_extension(const struct netmd_track_info *info)
{
    if (info->encoding == NETMD_ENCODING_SP) {
        return ".aea";
    } else {
        return ".wav";
    }
}

char *
netmd_recv_get_default_filename(const struct netmd_track_info *info)
{
    const char *ext = netmd_recv_get_file_extension(info);

    char buf[256];
    if (strlen(info->title) != 0) {
        snprintf(buf, sizeof(buf), "%02d %s%s", info->track_id + 1, info->title, ext);
    } else {
        snprintf(buf, sizeof(buf), "%02d Track %d%s", info->track_id + 1, info->track_id + 1, ext);
    }

    return strdup(buf);
}

netmd_error netmd_recv_track(netmd_dev_handle *devh, int track_id, const char *filename,
        netmd_recv_progress_func recv_progress, void *recv_progress_user_data)
{
    if (!netmd_dev_can_upload(devh)) {
        return NETMD_UNSUPPORTED_FEATURE;
    }

    if (filename == NULL) {
        return NETMD_ERROR;
    }

    FILE *fp = fopen(filename, "wb");
    int result = netmd_secure_recv_track(devh, track_id, fp,
            recv_progress, recv_progress_user_data);
    fclose(fp);

    return result;
}
