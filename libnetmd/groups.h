#pragma once

/** \file groups.h */

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

#include <stdint.h>

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
   Sets title for the specified track.

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param group Zero based index of group your renaming (zero is disc title).
   @param title buffer holding the name.
   @return returns 0 for fail 1 for success.
*/
int netmd_set_group_title(netmd_dev_handle* dev, minidisc* md, unsigned int group, const char* title);


/**
   Cleans memory allocated for the name of each group, then cleans groups
   pointer

   @param md pointer to minidisc structure
*/
void netmd_clean_disc_info(minidisc* md);


#ifdef __cplusplus
}
#endif
