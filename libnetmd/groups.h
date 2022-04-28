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

#include "common.h"
#include "error.h"

/**
   Data about a group, start track, finish track and name. Used to generate disc
   header info.
*/
typedef struct netmd_group {
    int first; /*!< First track index, 1-based; 0 for disc title and empty groups */
    int last; /*!< First track index, 1-based, 0 for disc title and empty groups */
    char *name;
} netmd_group_t;

/**
   stores misc data for a minidisc
*/
typedef struct {
    struct netmd_group *groups;
    size_t group_count;
} minidisc;


/**
 * Load disc title and group information from disc.
 *
 * This loads the disc title and parses it into groups.
 *
 * @param dev Handle to netmd device
 * @return A newly-allocated Minidisc object, or NULL if parsing failed.
 * Free the returned value with netmd_minidisc_free() once you are done with it.
 */
minidisc *
netmd_minidisc_load(netmd_dev_handle *dev);

/**
 * Free a minidisc object obtained via netmd_minidisc_load().
 *
 * @param md Minidisc object
 */
void
netmd_minidisc_free(minidisc *md);


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
netmd_group_id
netmd_minidisc_get_track_group(const minidisc *md, netmd_track_index track_id);


/**
 * Get the name of a group.
 *
 * @param md Minidisc object
 * @param group 1-based group number (as returned by netmd_minidisc_get_track_group())
 * @return The name of the group, or NULL if the group does not exist
 */
const char *
netmd_minidisc_get_group_name(const minidisc *md, netmd_group_id group);


/**
 * Check if a group is empty (contains no tracks).
 *
 * @param md Minidisc object
 * @param group 1-based group number
 * @return true if the group contains no tracks, false otherwise
 */
bool
netmd_minidisc_is_group_empty(const minidisc *md, netmd_group_id group);


/**
 * Create a new empty group at the end of the disc.
 *
 * @param dev Device handle
 * @param md Minidisc object
 * @param name Name of the group to create
 * @return NETMD_NO_ERROR on success, or error code otherwise
 */
netmd_error
netmd_create_group(netmd_dev_handle *dev, minidisc *md, const char *name);


/**
 * Creates disc header out of groups and writes it to disc
 *
 * @param devh pointer to device returned by netmd_open
 * @param md pointer to minidisc structure
 * @return NETMD_NO_ERROR on success, or an error code
 */
netmd_error
netmd_write_disc_header(netmd_dev_handle* devh, const minidisc *md);

/**
   Deletes group from disc (but not the tracks in it)

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param group Zero based track to delete
*/
netmd_error
netmd_delete_group(netmd_dev_handle *dev, minidisc *md, netmd_group_id group);

/**
   Sets title for the specified track.

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param group Zero based index of group your renaming (zero is disc title).
   @param title buffer holding the name.
   @return NETMD_NO_ERROR on success, or an error code
*/
netmd_error
netmd_set_group_title(netmd_dev_handle *dev, minidisc *md, netmd_group_id group, const char *title);

/**
 * Update the track range of a group.
 *
 * The new group range must not overlap any existing groups.
 *
 * @param dev Device handle
 * @param md Minidisc object
 * @param first 1-based track index of first track in group, or 0 to make the group empty
 * @param last 1-based track index of last track in group (ignored if first == 0)
 * @return NETMD_NO_ERROR on success, or an error code
 */
netmd_error
netmd_update_group_range(netmd_dev_handle *dev, minidisc *md, netmd_group_id group, int first, int last);

#ifdef __cplusplus
}
#endif
