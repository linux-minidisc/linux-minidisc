#pragma once

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

/**
   Data about a group, start track, finish track and name. Used to generate disc
   header info.
*/
typedef struct netmd_group
{
    uint16_t start;
    uint16_t finish;
    char* name;
} netmd_group_t;

/**
   Basic track data.
*/
struct netmd_track
{
    int track;
    int minute;
    int second;
    int tenth;
};

/**
   stores misc data for a minidisc
*/
typedef struct {
    size_t header_length;
    struct netmd_group *groups;
    unsigned int group_count;
} minidisc;

/**
 * Get the disc name of a minidisc.
 *
 * @param md Minidisc object
 * @return A string representing the disc name.
 */
const char *
netmd_minidisc_get_disc_name(const minidisc *md);


/**
 * Get the group a track is in.
 *
 * @param md Minidisc object
 * @param track_id Zero-based track number.
 * @return 0 if the track is in no group, or the group number (1-based).
 */
int
netmd_minidisc_get_track_group(const minidisc *md, uint16_t track_id);


/**
 * Get the name of a group.
 *
 * @param md Minidisc object
 * @param group 1-based group number (as returned by netmd_minidisc_get_track_group())
 * @return The name of the group, or NULL if the group does not exist
 */
const char *
netmd_minidisc_get_group_name(const minidisc *md, int group);


/**
 * Check if a group is empty (contains no tracks).
 *
 * @param md Minidisc object
 * @param group 1-based group number
 * @return true if the group contains no tracks, false otherwise
 */
bool
netmd_minidisc_is_group_empty(const minidisc *md, int group);


/**
   Global variable containing netmd_group data for each group. There will be
   enough for group_count total in the alloced memory
*/
extern struct netmd_group* groups;

/**
 * Return a string representation of the encoding name.
 *
 * @param encoding The encoding, NETMD_ENCODING_SP, NETMD_ENCODING_LP2 or NETMD_ENCODING_LP4
 * @return "SP", "LP2" or "LP4" or "UNKNOWN"
 **/
const char *netmd_get_encoding_name(enum NetMDEncoding encoding);

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
    const char *title; // With any "LP:" prefix stripped for LP2/LP4 tracks
    struct netmd_track duration;
    enum NetMDEncoding encoding;
    enum NetMDChannels channels;
    enum NetMDTrackFlags protection;

    // Storage for the actual title as present on disk
    char raw_title[256];
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
netmd_error netmd_get_track_info(netmd_dev_handle *dev, uint16_t track_id, struct netmd_track_info *info);


int netmd_request_track_time(netmd_dev_handle* dev, const uint16_t track, struct netmd_track* buffer);

/**
   Sets title for the specified track. If making multiple changes,
   call netmd_cache_toc before netmd_set_title and netmd_sync_toc
   afterwards.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer buffer to hold the name.
   @return returns 0 for fail 1 for success.
*/
int netmd_set_title(netmd_dev_handle* dev, const uint16_t track, const char* const buffer);

/**
   Sets title for the specified track.

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param group Zero based index of group your renaming (zero is disc title).
   @param title buffer holding the name.
   @return returns 0 for fail 1 for success.
*/
int netmd_set_group_title(netmd_dev_handle* dev, minidisc* md, unsigned int group, const char* title);

/**
   Moves track around the disc.

   @param dev pointer to device returned by netmd_open
   @param start Zero based index of track to move
   @param finish Zero based track to make it
   @return 0 for fail 1 for success
*/
int netmd_move_track(netmd_dev_handle* dev, const uint16_t start, const uint16_t finish);

/**
   sets up buffer containing group info.

  @param dev pointer to device returned by netmd_open
  @param md pointer to minidisc structure
  @return total size of disc header Group[0] is disc name.  You need to make
          sure you call clean_disc_info before recalling
*/
int netmd_initialize_disc_info(netmd_dev_handle* dev, minidisc* md);

void netmd_parse_disc_title(minidisc* md, char* title, size_t title_length);

void netmd_parse_group(minidisc* md, char* group, int* group_count);

void netmd_parse_trackinformation(minidisc* md, char* group_name, int* group_count, char* tracks);

int netmd_create_group(netmd_dev_handle* dev, minidisc* md, const char* name);

int netmd_set_disc_title(netmd_dev_handle* dev, const char* title, size_t title_length);

/**
   Creates disc header out of groups and writes it to disc

   @param devh pointer to device returned by netmd_open
   @param md pointer to minidisc structure
*/
int netmd_write_disc_header(netmd_dev_handle* devh, minidisc *md);

/**
   Moves track into group

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param track Zero based track to add to group.
   @param group number of group (0 is title group).
*/
int netmd_put_track_in_group(netmd_dev_handle* dev, minidisc* md, const uint16_t track, const unsigned int group);

/**
   Moves group around the disc.

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param track Zero based track to make group start at.
   @param group number of group (0 is title group).
*/
int netmd_move_group(netmd_dev_handle* dev, minidisc* md, const uint16_t track, const unsigned int group);

/**
   Deletes group from disc (but not the tracks in it)

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param group Zero based track to delete
*/
int netmd_delete_group(netmd_dev_handle* dev, minidisc* md, const unsigned int group);

/**
   Deletes track from disc (does not update groups)

   @param dev pointer to device returned by netmd_open
   @param track Zero based track to delete
*/
int netmd_delete_track(netmd_dev_handle* dev, const uint16_t track);

/**
   Erase all disc contents

   @param dev pointer to device returned by netmd_open
*/
int netmd_erase_disc(netmd_dev_handle* dev);

/**
   Writes atrac file to device

   @param dev pointer to device returned by netmd_open
   @param szFile Full path to file to write.
   @return < 0 on fail else 1
   @bug doesnt work yet
*/
int netmd_write_track(netmd_dev_handle* dev, char* szFile);

/**
   Cleans memory allocated for the name of each group, then cleans groups
   pointer

   @param md pointer to minidisc structure
*/
void netmd_clean_disc_info(minidisc* md);


/**
   sets group data

   @param md
   @param group
   @param name
   @param start
   @param finish
*/
/* void set_group_data(minidisc* md, int group, char* name, int start, int finish);*/

/**
   Sends a command to the MD unit and compare the result with response unless
   response is NULL

   @param dev a handler to the usb device
   @param str the string that should be sent
   @param len length of the string
   @param response string of the expected response. NULL for no expectations.
   @param length of the expected response
   @return the response. NOTE: this has to be freed up after calling.
*/
/* char* sendcommand(netmd_dev_handle* dev, char* str, int len, char* response, int rlen);*/

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
