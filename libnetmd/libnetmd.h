/* libnetmd.h 
 *      Copyright (C) 2002, 2003 Marc Britten
 *      
 * This file is part of libnetmd.
 *
 * libnetmd is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Libnetmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#include <usb.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h> 

#define DEVICE_COUNT 7
#define	CODECS 2
#define BITRATES 3

/** Struct to hold the vendor and product id's for each unit. */
struct netmd_devices {
	int	idVendor;
	int	idProduct;
};

/** Data about a group, start track, finish track and name.
  Used to generate disc header info.
*/
typedef struct netmd_group
{
	int start;
	int finish;
	char* name;
} netmd_group_t;

/** Basic track data. */
struct netmd_track
{
	int track;
	int minute;
	int second;
	int tenth;
};

/** stores hex value from protocol and text value of name */
typedef struct netmd_pair
{
	unsigned char hex;
	char* name;
} netmd_pair_t;

/** stores misc data for a minidisc **/
typedef struct {
	int header_length;
	struct netmd_group *groups;
	int group_count;
} minidisc;


/** Global variable containing netmd_group data for each group
	There will be enough for group_count total in the alloced memory
*/
extern struct netmd_group* groups;
extern struct netmd_pair const codecs[CODECS + 1];
extern struct netmd_pair const bitrates[BITRATES + 1];
extern struct netmd_pair const unknown_pair;

/** Utility function for checking data.
  \param buf buffer to print.
  \param size length of buffer to print.
*/
void print_hex(unsigned char* buf, size_t size);

/** enum through an array of pairs looking for a specific hex code.
	\param hex hex code to find.
	\pair array of pairs to look through.
*/
struct netmd_pair const* find_pair(int hex, struct netmd_pair const* pair);

/** used internally to get the size of buffer needed for the returning data.
	\param dev USB device handle.
	\return size of buffer to alloc.
*/
unsigned int request_buffer_length(usb_dev_handle* dev);

/** Finds netmd device and returns pointer to its device handle */
struct usb_device* init_netmd();

/*! Initialize USB subsystem for talking to NetMD
  \param dev Pointer returned by init_netmd.
*/
usb_dev_handle* open_netmd(struct usb_device* dev);

/*! Get the device name stored in USB device 
  \param dev pointer to device returned by open_netmd
  \param buf buffer to hold the name.
  \param buffsize of buf.
  \return Actual size of buffer, if your buffer is too small resize buffer and recall function.
*/
int get_devname(usb_dev_handle* dev, unsigned char* buf, int buffsize);

/*! Function for internal use by init_disc_info */
int request_disc_title(usb_dev_handle* dev, char* buffer, int size);

/*! Get the codec used to encode a specific track.
  \param dev pointer to device returned by open_netmd
  \param track Zero based index of track your requesting.
  \param data pointer to store the hex code representing the codec.
*/
int request_track_codec(usb_dev_handle*dev, int track, char* data);

/*! Get the bitrate used to encode a specific track.
  \param dev pointer to device returned by open_netmd
  \param track Zero based index of track your requesting.
  \param data pointer to store the hex code representing the bitrate.
*/
int request_track_bitrate(usb_dev_handle*dev, int track, unsigned char* data);

/*! Get the title for a specific track.
  \param dev pointer to device returned by open_netmd
  \param track Zero based index of track your requesting.
  \param buffer buffer to hold the name.
  \param size of buf.
  \return Actual size of buffer, if your buffer is too small resize buffer and recall function.
*/
int request_title(usb_dev_handle* dev, int track, char* buffer, int size);

int request_track_time(usb_dev_handle* dev, int track, struct netmd_track* buffer);

/*! Sets title for the specified track.
  \param dev pointer to device returned by open_netmd
  \param track Zero based index of track your requesting.
  \param buffer buffer to hold the name.
  \param size of buf.
  \return returns 0 for fail 1 for success.
*/
int set_title(usb_dev_handle* dev, int track, char* buffer, int size);

/*! Sets title for the specified track.
  \param dev pointer to device returned by open_netmd
  \param md pointer to minidisc structure
  \param group Zero based index of group your renaming (zero is disc title).
  \param title buffer holding the name.
  \return returns 0 for fail 1 for success.
*/
int set_group_title(usb_dev_handle* dev, minidisc* md, int group, char* title);

/*! Moves track around the disc.
  \param dev pointer to device returned by open_netmd
  \param start Zero based index of track to move
  \param finish Zero based track to make it
  \return 0 for fail 1 for success
*/
int move_track(usb_dev_handle* dev, int start, int finish);

/*! used internally - use the int group_count var.
  \param dev pointer to device returned by open_netmd
  \param md pointer to minidisc structure
  \return Number of groups, counting disc title as a group (which it is)
*/
int get_group_count(usb_dev_handle* dev, minidisc* md);

/*! sets up buffer containing group info.
  \param dev pointer to device returned by open_netmd
  \param md pointer to minidisc structure
  \return total size of disc header
  Group[0] is disc name.  You need to make sure you call clean_disc_info before recalling
*/
int initialize_disc_info(usb_dev_handle* dev, minidisc* md);

int create_group(usb_dev_handle* devh, char* name);

/*! Creates disc header out of groups and writes it to disc
  \param dev pointer to device returned by open_netmd
  \param md pointer to minidisc structure
*/
int write_disc_header(usb_dev_handle* devh, minidisc *md);

/*! Moves track into group
  \param dev pointer to device returned by open_netmd
  \param md pointer to minidisc structure
  \param track Zero based track to add to group.
  \param group number of group (0 is title group).
*/
int put_track_in_group(usb_dev_handle* dev, minidisc* md, int track, int group);

/*! Moves group around the disc.
  \param dev pointer to device returned by open_netmd
  \param md pointer to minidisc structure
  \param track Zero based track to make group start at.
  \param group number of group (0 is title group).
*/
int move_group(usb_dev_handle* dev, minidisc* md, int track, int group);

/*! Deletes group from disc (but not the tracks in it)
  \param dev pointer to device returned by open_netmd
  \param md pointer to minidisc structure
  \param track Zero based track to delete
*/
int delete_group(usb_dev_handle* dev, minidisc* md, int group);

int netmd_set_track(usb_dev_handle* dev, int track);
int netmd_play(usb_dev_handle* dev);
int netmd_stop(usb_dev_handle* dev);
int netmd_pause(usb_dev_handle* dev);
int netmd_rewind(usb_dev_handle* dev);
int netmd_fast_forward(usb_dev_handle* dev);

int netmd_delete_track(usb_dev_handle* dev, int track);

/*! Writes atrac file to device
  \param dev pointer to device returned by open_netmd
  \param szFile Full path to file to write.
  \return < 0 on fail else 1
  \bug doesnt work yet
*/
int write_track(usb_dev_handle* dev, char* szFile);

/*! Cleans memory allocated for the name of each group, then cleans groups pointer
  \param md pointer to minidisc structure
*/
void clean_disc_info(minidisc* md);

/*! closes the usb descriptors 
  \param dev pointer to device returned by open_netmd
*/
void clean_netmd(usb_dev_handle* dev);
void test(usb_dev_handle* dev);

/*! gets the position within the currently playing track in seconds.hundreds
  \param dev pointer to device returned by open_netmd
 */
float netmd_get_playback_position(usb_dev_handle* dev);

/*! gets the currently playing track
  \param dev pointer to device returned by open_netmd
 */
int netmd_get_current_track(usb_dev_handle* dev);

/*! sets group data
  \param md
  \param group
  \param name
  \param start
  \param finish
*/
void set_group_data(minidisc* md, int group, char* name, int start, int finish);

/*! Sends a command to the MD unit and compare the result with response unless response is NULL 
  \param dev a handler to the usb device
  \param str the string that should be sent
  \param len length of the string
  \param response string of the expected response. NULL for no expectations.
  \param length of the expected response
  \return the response. NOTE: this has to be freed up after calling.
*/
char* sendcommand(usb_dev_handle* dev, char* str, int len, char* response, int rlen);

/*! Wait for syncronisation signal from minidisc 
  \param dev a handler to the usb device
*/ 
void waitforsync(usb_dev_handle* dev);

