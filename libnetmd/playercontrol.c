/* playercontrol.c
 *      Copyright (C) 2004, Bertrik Sikken
 *      Copyright (C) 2011, Alexander Sulfrian
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

#include "playercontrol.h"
#include "utils.h"
#include "const.h"


static int netmd_playback_control(netmd_dev_handle* dev, unsigned char code)
{
    char request[] = {0x00, 0x18, 0xc3, 0xff, 0x00, 0x00,
                      0x00, 0x00};
    char buf[255];
    int size;

    request[4] = code;
    size = netmd_exch_message(dev, request, sizeof(request), buf);

    if (size < 1) {
        return -1;
    }

    return 1;
}

int netmd_play(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_PLAY);
}

int netmd_pause(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_PAUSE);
}

int netmd_fast_forward(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_FFORWARD);
}

int netmd_rewind(netmd_dev_handle *dev)
{
    return netmd_playback_control(dev, NETMD_REWIND);
}

int netmd_stop(netmd_dev_handle* dev)
{
    char request[] = {0x00, 0x18, 0xc5, 0xff, 0x00, 0x00,
                      0x00, 0x00};
    char buf[255];
    int size;

    size = netmd_exch_message(dev, request, sizeof(request), buf);

    if(size < 0) {
        return 0;
    }

    if (size < 1) {
        return -1;
    }

    return 1;
}

int netmd_set_playmode(netmd_dev_handle* dev, int playmode)
{
    char request[] = {0x00, 0x18, 0xd1, 0xff, 0x01, 0x00,
                      0x00, 0x00, 0x88, 0x08, 0x00, 0x00,
                      0x00};
    char buf[255];
    int size;

    request[10] = (playmode >> 8) & 0xFF;
    request[11] = (playmode >> 0) & 0xFF;

    size = netmd_exch_message(dev, request, sizeof(request), buf);
    return size;
}

int netmd_set_track( netmd_dev_handle* dev, int track)
{
    char request[] = {0x00, 0x18, 0x50, 0xff, 0x01, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00};
    char buf[255];
    int size;

    proper_to_bcd(track, request + 9, 2);

    size = netmd_exch_message(dev, request, sizeof(request), buf);

    if(size < 0) {
        return 0;
    }

    if (size < 1) {
        return -1;
    }

    return 1;
}

int netmd_change_track(netmd_dev_handle* dev, int direction)
{
    char request[] = {0x00, 0x18, 0x50, 0xff, 0x10, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00};
    char buf[255];
    int size;

    request[9] = (direction >> 8) & 0xFF;
    request[10] = (direction >> 0) & 0xFF;

    size = netmd_exch_message(dev, request, sizeof(request), buf);
    return size;
}

int netmd_get_track(netmd_dev_handle* dev)
{
    char request[] = {0x00, 0x18, 0x09, 0x80, 0x01, 0x04,
                      0x30, 0x88, 0x02, 0x00, 0x30, 0x88,
                      0x05, 0x00, 0x30, 0x00, 0x03, 0x00,
                      0x30, 0x00, 0x02, 0x00, 0xff, 0x00,
                      0x00, 0x00, 0x00, 0x00};
    char buf[255];
    int track = 0;

    netmd_exch_message(dev, request, 28, buf);
    track = bcd_to_proper(buf + 35, 2);

    return track;
}

int netmd_track_next(netmd_dev_handle* dev)
{
    return netmd_change_track(dev, NETMD_TRACK_NEXT);
}

int netmd_track_previous(netmd_dev_handle* dev)
{
    return netmd_change_track(dev, NETMD_TRACK_PREVIOUS);
}

int netmd_track_restart(netmd_dev_handle* dev)
{
    return netmd_change_track(dev, NETMD_TRACK_RESTART);
}

int netmd_set_time(netmd_dev_handle* dev, int track, const netmd_time* time)
{
    char request[] = {0x00, 0x18, 0x50, 0xff, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                      0x00, 0x00, 0x00};
    char buf[255];
    int size;

    proper_to_bcd(track, request + 9, 2);
    proper_to_bcd(time->hour, request + 11, 1);
    proper_to_bcd(time->minute, request + 12, 1);
    proper_to_bcd(time->second, request + 13, 1);
    proper_to_bcd(time->frame, request + 14, 1);

    size = netmd_exch_message(dev, request, sizeof(request), buf);

    if(size < 0) {
        return 0;
    }

    if (size < 1) {
        return -1;
    }

    return 1;
}

const netmd_time* netmd_parse_time(char* src, netmd_time* time)
{
    time->hour = bcd_to_proper(src, 2);
    time->minute = bcd_to_proper(src + 2, 1);
    time->second = bcd_to_proper(src + 3, 1);
    time->frame = bcd_to_proper(src + 4, 1);

    return time;
}

const netmd_time* netmd_get_position(netmd_dev_handle* dev, netmd_time* time)
{
    char request[] = {0x00, 0x18, 0x09, 0x80, 0x01, 0x04,
                      0x30, 0x88, 0x02, 0x00, 0x30, 0x88,
                      0x05, 0x00, 0x30, 0x00, 0x03, 0x00,
                      0x30, 0x00, 0x02, 0x00, 0xff, 0x00,
                      0x00, 0x00, 0x00, 0x00};
    char buf[255];
    int ret = 0;

    ret = netmd_exch_message(dev, request, sizeof(request), buf);
    time->hour = bcd_to_proper(buf + 37, 1);
    time->minute = bcd_to_proper(buf + 38, 1);
    time->second = bcd_to_proper(buf + 39, 1);
    time->frame = bcd_to_proper(buf + 40, 1);

    return time;
}

/**
 * Get disc capacity.
 * Returns a list of 3 lists of 4 elements each (see getTrackLength).
 * The first list is the recorded duration.
 * The second list is the total disc duration (*).
 * The third list is the available disc duration (*).
 * (*): This result depends on current recording parameters.
 */
const netmd_disc_capacity* netmd_get_disc_capacity(netmd_dev_handle* dev, netmd_disc_capacity* capacity)
{
    char request[] = {0x00, 0x18, 0x06, 0x02, 0x10, 0x10,
                      0x00, 0x30, 0x80, 0x03, 0x00, 0xff,
                      0x00, 0x00, 0x00, 0x00, 0x00};
    char buf[255];
    int ret = 0;

    ret = netmd_exch_message(dev, request, sizeof(request), buf);
    netmd_parse_time(buf + 27, &capacity->recorded);
    netmd_parse_time(buf + 34, &capacity->total);
    netmd_parse_time(buf + 41, &capacity->available);

    return capacity;
}
