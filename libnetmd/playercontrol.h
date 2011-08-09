#ifndef LIBNETMD_PLAYERCONTROL_H
#define LIBNETMD_PLAYERCONTROL_H

#include <stdint.h>
#include "common.h"

typedef struct {
    uint16_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t frame;
} netmd_time;

typedef struct {
    netmd_time recorded;
    netmd_time total;
    netmd_time available;
} netmd_disc_capacity;

int netmd_play(netmd_dev_handle* dev);
int netmd_pause(netmd_dev_handle* dev);
int netmd_fast_forward(netmd_dev_handle* dev);
int netmd_rewind(netmd_dev_handle* dev);
int netmd_stop(netmd_dev_handle* dev);

int netmd_set_playmode(netmd_dev_handle* dev, int playmode);
int netmd_set_track(netmd_dev_handle* dev, int track);

/*
  gets the currently playing
  @param dev pointer to device returned by netmd_open
*/
int netmd_get_track(netmd_dev_handle* dev);

int netmd_track_next(netmd_dev_handle* dev);
int netmd_track_previous(netmd_dev_handle* dev);
int netmd_track_restart(netmd_dev_handle* dev);

int netmd_set_time(netmd_dev_handle* dev, int track, const netmd_time* time);

/*
  gets the position within the currently playing track in seconds.hundreds
  @param dev pointer to device returned by netmd_open
*/
const netmd_time* netmd_get_position(netmd_dev_handle* dev, netmd_time* time);

const netmd_disc_capacity* netmd_get_disc_capacity(netmd_dev_handle* dev, netmd_disc_capacity* capacity);

#endif
