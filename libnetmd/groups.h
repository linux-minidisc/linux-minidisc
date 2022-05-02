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
 * Load disc title and group information from disc.
 *
 * This loads the disc title and parses it into groups.
 *
 * Normally this will be called automatically, but you can
 * force a reload of the disc header by calling this function.
 *
 * @param dev NetMD device handle
 * @return NETMD_NO_ERROR on success, or an error code
 */
netmd_error
netmd_load_disc_header(netmd_dev_handle *dev);


/**
 * Creates disc header out of groups and writes it to disc
 *
 * Normally this will be called automatically, but you can
 * force a write of the disc header by calling this function.
 *
 * @param dev NetMD device handle
 * @return NETMD_NO_ERROR on success, or an error code
 */
netmd_error
netmd_write_disc_header(netmd_dev_handle *dev);


/**
 * Clear the disc header.
 *
 * This clears the internal structure used for the disc
 * title and groups.
 *
 * Normally this will be called automatically, but you can
 * clear the disc header, for example if the disk is changed.
 *
 * @param dev NetMD device handle
 */
void
netmd_clear_disc_header(netmd_dev_handle *dev);


/**
 * Get the disc title of a minidisc.
 *
 * @param dev NetMD device handle
 * @return A string representing the disc name.
 */
const char *
netmd_get_disc_title(netmd_dev_handle *dev);


/**
 * Get the group a track is in.
 *
 * @param dev NetMD device handle
 * @param track_id Zero-based track number.
 * @return 0 if the track is in no group, or the group number (1-based).
 */
netmd_group_id
netmd_get_track_group(netmd_dev_handle *dev, netmd_track_index track_id);


/**
 * Get the name of a group.
 *
 * @param dev NetMD device handle
 * @param group 1-based group number (as returned by netmd_get_track_group())
 * @return The name of the group, or NULL if the group does not exist
 */
const char *
netmd_get_group_name(netmd_dev_handle *dev, netmd_group_id group);


/**
 * Check if a group is empty (contains no tracks).
 *
 * @param dev NetMD device handle
 * @param group 1-based group number
 * @return true if the group contains no tracks, false otherwise
 */
bool
netmd_is_group_empty(netmd_dev_handle *dev, netmd_group_id group);


/**
 * Create a new empty group at the end of the disc.
 *
 * @param dev NetMD device handle
 * @param name Name of the group to create
 * @return NETMD_NO_ERROR on success, or error code otherwise
 */
netmd_error
netmd_create_group(netmd_dev_handle *dev, const char *name);

/**
   Deletes group from disc (but not the tracks in it)

 * @param dev NetMD device handle
   @param md pointer to minidisc structure
   @param group Zero based track to delete
*/
netmd_error
netmd_delete_group(netmd_dev_handle *dev, netmd_group_id group);

/**
   Sets title for the specified group.

 * @param dev NetMD device handle
   @param group Zero based index of group your renaming (zero is disc title).
   @param title buffer holding the name.
   @return NETMD_NO_ERROR on success, or an error code
*/
netmd_error
netmd_set_group_title(netmd_dev_handle *dev, netmd_group_id group, const char *title);

/**
 * Set the disc title.
 *
 * Set the disc title, taking groups into account. The written raw disc
 * title will contain the group information stored in the device handle.
 *
 * @param dev NetMD device handle
 * @param title New title to set.
 * @return NETMD_NO_ERROR on success, or an error code
 */
netmd_error
netmd_set_disc_title(netmd_dev_handle *dev, const char *title);

/**
 * Update the track range of a group.
 *
 * The new group range must not overlap any existing groups.
 *
 * @param dev NetMD device handle
 * @param first 1-based track index of first track in group, or 0 to make the group empty
 * @param last 1-based track index of last track in group (ignored if first == 0)
 * @return NETMD_NO_ERROR on success, or an error code
 */
netmd_error
netmd_update_group_range(netmd_dev_handle *dev, netmd_group_id group, int first, int last);

#ifdef __cplusplus
}
#endif
