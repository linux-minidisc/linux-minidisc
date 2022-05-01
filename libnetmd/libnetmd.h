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
#include "recv.h"
#include "query.h"
#include "descriptor.h"

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
   Sets title for the specified track.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer String containing the new track name.
   @return NETMD_NO_ERROR on success, or an error code
*/
netmd_error
netmd_set_track_title(netmd_dev_handle* dev, netmd_track_index track, const char *buffer);

/**
   Moves track around the disc. (does not update groups)

   @param dev pointer to device returned by netmd_open
   @param start Zero based index of track to move
   @param finish Zero based track to make it
   @return 0 for fail 1 for success
*/
int netmd_move_track(netmd_dev_handle* dev, const uint16_t start, const uint16_t finish);

/**
 * Set the complete (raw) title of the disc.
 *
 * Any custom group information will be overwritten. To set the disc title
 * without affecting groups, use netmd_set_group_title() with group 0.
 *
 * @param dev Handle to the device
 * @param title The new title
 */
int netmd_set_disc_title(netmd_dev_handle* dev, const char* title);

/**
 * Get the number of tracks on the disc.
 *
 * @param dev Handle to the device
 * @return Number of tracks, or -1 on error
 */
int
netmd_get_track_count(netmd_dev_handle *dev);

/**
 * Get the complete (raw) title of the disc.
 *
 * To get the parsed title of the disc, taking groups into account,
 * use netmd_minidisc_get_disc_name().
 *
 * @param dev Handle to the device
 * @param buffer Buffer to store the title
 * @param size Size of the buffer
 * @return Number of bytes written to buffer, or -1
 */
ssize_t
netmd_get_disc_title(netmd_dev_handle* dev, char* buffer, size_t size);

/**
   Deletes track from disc (does not update groups)

   @param dev pointer to device returned by netmd_open
   @param track Zero based track to delete
*/
int netmd_delete_track(netmd_dev_handle* dev, netmd_track_index track);

/**
 * Check if the inserted disc is writable or not.
 *
 * @param dev USB device handle
 * @return true if the disc is writable, false otherwise
 */
bool
netmd_is_disc_writable(netmd_dev_handle *dev);

/**
   Erase all disc contents

   Call netmd_secure_leave_session() afterwards
   to let the recorder commit the changes to disc.

   @param dev pointer to device returned by netmd_open
   @return NETMD_NO_ERROR on success, or an error code
*/
netmd_error
netmd_erase_disc(netmd_dev_handle *dev);

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
 * Format a NetMD time to a string.
 *
 * @param time A pointer to a netmd_time structure
 * @return A newly-allocated string, free with netmd_free_string()
 */
char *netmd_time_to_string(const netmd_time *time);


/**
 * Format a NetMD UUID to a string.
 *
 * @param uuid A pointer to a netmd_uuid structure
 * @return A newly-allocated string, free with netmd_free_string()
 */
char *
netmd_uuid_to_string(const netmd_uuid *uuid);


/**
 * Free a string previously returned.
 *
 * @param string An allocated string to be free'd
 */
void netmd_free_string(char *string);


/**
 * Send a query to the device and read the response.
 *
 * Ownership of the query object will be taken over by this function, so
 * you must not free the query object separately.
 *
 * @param dev USB device handle
 * @param query The query to send to the device
 * @return A byte buffer containing the reponse, or NULL if there was an error
 */
struct netmd_bytebuffer *
netmd_send_query(netmd_dev_handle *dev, struct netmd_bytebuffer *query);

#ifdef __cplusplus
}
#endif
