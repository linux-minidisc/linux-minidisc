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
#include <stdbool.h>

#include <libusb.h>

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
   stores hex value from protocol and text value of name
*/
typedef struct netmd_pair
{
    unsigned char hex;
    const char* const name;
} netmd_pair_t;

/**
   stores misc data for a minidisc
*/
typedef struct {
    size_t header_length;
    struct netmd_group *groups;
    unsigned int group_count;
} minidisc;


/**
   Global variable containing netmd_group data for each group. There will be
   enough for group_count total in the alloced memory
*/
extern struct netmd_group* groups;
extern struct netmd_pair const trprot_settings[];
extern struct netmd_pair const bitrates[];
extern struct netmd_pair const unknown_pair;

/**
   enum through an array of pairs looking for a specific hex code.
   @param hex hex code to find.
   @param pair array of pairs to look through.
*/
struct netmd_pair const* find_pair(int hex, struct netmd_pair const* pair);

int netmd_request_track_time(netmd_dev_handle* dev, const uint16_t track, struct netmd_track* buffer);

/**
   Sets title for the specified track.

   Titles set using this function are converted from
   UTF-8 for your convenience.

   Note that this function will automatically choose the
   most appropriate title field to use, and clear the other.
   For specific control over the wide and narrow title fields,
   please use netmd_set_title_narrow and netmd_set_title_wide.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer buffer to hold the name.
   @return returns 0 for fail 1 for success.
*/
int netmd_set_title(netmd_dev_handle* dev, const uint16_t track, const char* const buffer);

/**
   Sets narrow (JIS X0201) title for the specified track.

   Titles set using this function are converted from
   UTF-8 for your convenience.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer buffer to hold the name.
   @return returns -1 if text couldn't be converted, 0 for failure, and 1 for success.
*/
int netmd_set_title_narrow(netmd_dev_handle* dev, const uint16_t track, const char* const buffer);

/**
   Sets wide (Shift JIS) title for the specified track.

   Titles set using this function are converted from
   UTF-8 for your convenience.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer buffer to hold the name.
   @return returns -1 if text couldn't be converted, 0 for failure, and 1 for success.
*/
int netmd_set_title_wide(netmd_dev_handle* dev, const uint16_t track, const char* const buffer);

/**
   Sets raw title for the specified track.

   Narrow titles are expected to be raw JIS X0201 text,
   wide titles are expected to be raw Shift JIS text.
   A given MiniDisc track may only have one set.

   For automatic selection, and UTF-8 conversion,
   please use netmd_set_title instead.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer buffer to hold the name.
   @param whether to set the narrow or wide title.
   @return returns 0 for fail 1 for success.
*/
int netmd_set_title_raw(netmd_dev_handle* dev, const uint16_t track, const char* const buffer, bool wide_chars);

/**
   Sets title for the specified group.

   @param dev pointer to device returned by netmd_open
   @param md pointer to minidisc structure
   @param group Zero based index of group your renaming (zero is disc title).
   @param title buffer holding the name.
   @return returns 0 for fail 1 for success.
*/
int netmd_set_group_title(netmd_dev_handle* dev, minidisc* md, unsigned int group, char* title);

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

int netmd_create_group(netmd_dev_handle* dev, minidisc* md, char* name);

int netmd_set_disc_title(netmd_dev_handle* dev, char* title, size_t title_length);

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

int netmd_delete_track(netmd_dev_handle* dev, const uint16_t track);

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

/**
   Wait for syncronisation signal from minidisc

   @param dev a handler to the usb device
*/
/* void waitforsync(netmd_dev_handle* dev);*/

int netmd_cache_toc(netmd_dev_handle* dev);
int netmd_sync_toc(netmd_dev_handle* dev);
