#pragma once

/** \file libnetmd.h */

/*
 * libnetmd.h
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "const.h"
#include "error.h"
#include "common.h"
#include "playercontrol.h"
#include "log.h"
#include "secure.h"
#include "netmd_dev.h"
#include "trackinformation.h"
#include "groups.h"

/**
   Basic track data.
*/
struct netmd_track
{
    netmd_track_index track_id;
    int minute;
    int second;
    int tenth;
};

/**
   Global variable containing netmd_group data for each group. There will be
   enough for group_count total in the alloced memory
*/
extern struct netmd_group* groups;

/**
 * Return a string representation of the encoding name.
 *
 * @param encoding The encoding, NETMD_ENCODING_SP, NETMD_ENCODING_LP2 or NETMD_ENCODING_LP4
 * @param channels The channels, NETMD_CHANNELS_STEREO or NETMD_CHANNELS_MONO
 * @return "SP", "LP2", "LP4", "Mono" or "UNKNOWN"
 **/
const char *netmd_get_encoding_name(enum NetMDEncoding encoding, enum NetMDChannels channels);

/**
 * Return a string representation of the track flag.
 *
 * @param flags The flags, as returned by netmd_request_track_flags()
 * @return "UnPROT", "TrPROT" or "UNKNOWN"
 **/
const char *netmd_track_flags_to_string(enum NetMDTrackFlags flags);


/**
 * Struct filled in by netmd_get_track_info(), see there.
 */
struct netmd_track_info {
    const char *title; /*!< User-visible track title, with any "LP:" prefix stripped for LP2/LP4 tracks */
    struct netmd_track duration; /*!< Duration of the track (in wall time) */
    enum NetMDEncoding encoding; /*!< Encoding used (SP, LP2, LP4) */
    enum NetMDChannels channels; /*!< For SP tracks, whether it is encoded as Mono or Stereo */
    enum NetMDTrackFlags protection; /*!< DRM status of the track (for upload and deletion) */

    char raw_title[256]; /*!< Storage for the actual title (including any "LP:" prefixes) as present on disk */
};


/**
 * Query track information for a track.
 *
 * This queries all track infos in one go, and fills a netmd_track_info struct.
 *
 * @param dev Handle to the NetMD device
 * @param track_id Zero-based track number
 * @param info Pointer to struct that will be filled
 * @return NETMD_NO_ERROR on success, or an error code on failure
 */
netmd_error netmd_get_track_info(netmd_dev_handle *dev, netmd_track_index track_id, struct netmd_track_info *info);


int netmd_request_track_time(netmd_dev_handle* dev, netmd_track_index track, struct netmd_track* buffer);

/**
   Sets title for the specified track. If making multiple changes,
   call netmd_cache_toc before netmd_set_title and netmd_sync_toc
   afterwards.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer buffer to hold the name.
   @return returns 0 for fail 1 for success.
*/
int netmd_set_title(netmd_dev_handle* dev, netmd_track_index track, const char* const buffer);

/**
   Moves track around the disc.

   @param dev pointer to device returned by netmd_open
   @param start Zero based index of track to move
   @param finish Zero based track to make it
   @return 0 for fail 1 for success
*/
int netmd_move_track(netmd_dev_handle* dev, const uint16_t start, const uint16_t finish);

int netmd_set_disc_title(netmd_dev_handle* dev, const char* title, size_t title_length);

/**
   Deletes track from disc (does not update groups)

   @param dev pointer to device returned by netmd_open
   @param track Zero based track to delete
*/
int netmd_delete_track(netmd_dev_handle* dev, netmd_track_index track);

/**
   Erase all disc contents

   @param dev pointer to device returned by netmd_open
*/
int netmd_erase_disc(netmd_dev_handle* dev);

int netmd_cache_toc(netmd_dev_handle* dev);
int netmd_sync_toc(netmd_dev_handle* dev);
int netmd_acquire_dev(netmd_dev_handle* dev);
int netmd_release_dev(netmd_dev_handle* dev);

/**
 * Download an audio file to the NetMD device.
 *
 * Supported file formats:
 *  - 16 bit pcm (stereo or mono) @44100Hz OR
 *  - Atrac LP2/LP4 data stored in a WAV container.
 *
 * If in_title is NULL, the filename (without path and extension) will be used as title.
 *
 * @param dev a handle to the USB device
 * @param filename Local filename of file to be sent
 * @param in_title Title to use for the track, or NULL
 * @param send_progress A callback for progress updates, or NULL
 * @param send_progress_user_data An opaque pointer for send_progress() (closure), or NULL
 */
netmd_error netmd_send_track(netmd_dev_handle *devh, const char *filename, const char *in_title,
        netmd_send_progress_func send_progress, void *send_progress_user_data);

/**
 * Upload an audio file from a MZ-RH1 in NetMD mode.
 *
 * @param devh A handle to the USB device
 * @param track_id Zero-based track index
 * @param filename Target filename, or NULL to generate a filename based on track title and format
 * @param recv_progress A callback for progress updates, or NULL
 * @param recv_progress_user_data An opaque pointer for recv_progress() (closure), or NULL
 */
netmd_error netmd_recv_track(netmd_dev_handle *devh, int track_id, const char *filename,
        netmd_recv_progress_func recv_progress, void *recv_progress_user_data);



/**
 * Format a netmd_track duration to a string.
 *
 * @param duration Pointer to a netmd_track duration struct
 * @return Newly-allocated string, free with netmd_free_string()
 */
char *netmd_track_duration_to_string(const struct netmd_track *duration);


/**
 * Format a NetMD time to a string.
 *
 * @param time A pointer to a netmd_time structure
 * @return A newly-allocated string, free with netmd_free_string()
 */
char *netmd_time_to_string(const netmd_time *time);


/**
 * Free a string previously returned.
 *
 * @param string An allocated string to be free'd
 */
void netmd_free_string(char *string);

#ifdef __cplusplus
}
#endif
