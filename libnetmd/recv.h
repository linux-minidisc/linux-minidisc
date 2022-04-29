#pragma once

/** \file recv.h */

/*
 * recv.h
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

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "secure.h"

/**
 * Get the desired file extension for a given track info.
 *
 * Depending on which codec is used, the container format for uploaded
 * files will be different.
 *
 * @param info A pointer to the track info for the track to be received
 * @return The file extension, ".aea" (for ATRAC1) or ".wav" (for ATRAC3)
 */
const char *
netmd_recv_get_file_extension(const struct netmd_track_info *info);

/**
 * Generate a default filename for a given track.
 *
 * This will generate a filename based on the track number, track title
 * and default file extension, to be passed to netmd_recv_track() as
 * filename.
 *
 * @return A newly-allocated string, free with netmd_free_string(), or NULL on error
 */
char *
netmd_recv_get_default_filename(const struct netmd_track_info *info);

/**
 * Upload an audio file from a MZ-RH1 in NetMD mode.
 *
 * @param devh A handle to the USB device
 * @param track_id Zero-based track index
 * @param filename Target filename, see netmd_recv_get_default_filename() to generate one
 * @param recv_progress A callback for progress updates, or NULL
 * @param recv_progress_user_data An opaque pointer for recv_progress() (closure), or NULL
 */
netmd_error
netmd_recv_track(netmd_dev_handle *devh, int track_id, const char *filename,
        netmd_recv_progress_func recv_progress, void *recv_progress_user_data);

#ifdef __cplusplus
}
#endif
