#ifndef LIBNETMD_PLAYERCONTROL_H
#define LIBNETMD_PLAYERCONTROL_H

#include <stdint.h>
#include "common.h"
#include "error.h"

typedef struct {
    uint16_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t frame;
} netmd_time;

/**
   Structure to hold the capacity information of a disc.
*/
typedef struct {
    /** Time allready recorded on the disc. */
    netmd_time recorded;

    /** Total time, that could be recorded on the disc. This depends on the
       current recording settings. */
    netmd_time total;

    /** Time that is available on the disc. This depends on the current
       recording settings. */
    netmd_time available;
} netmd_disc_capacity;

/**
   Starts playing the current track. If no track is selected, starts playing the
   first track.

   @param dev Handle to the open minidisc player.
*/
netmd_error netmd_play(netmd_dev_handle* dev);

/**
   Pause playing. If uses netmd_play afterwards the player continues at the
   current position.

   @param dev Handle to the open minidisc player.
*/
netmd_error netmd_pause(netmd_dev_handle* dev);

/**
   Spin the track fast forward.

   @param dev Handle to the open minidisc player.
*/
netmd_error netmd_fast_forward(netmd_dev_handle* dev);

/**
   Spin the track backwards in time (aka rewind it).

   @param dev Handle to the open minidisc player.
*/
netmd_error netmd_rewind(netmd_dev_handle* dev);

/**
   Stop playing. The current position is discarded.

   @param dev Handle to the open minidisc player.
*/
netmd_error netmd_stop(netmd_dev_handle* dev);

/**
   Set the playmode.

   @param dev Handle to the open minidisc player.
   @param playmode Playmode to set. Could be a OR'ed combination of the
                   corresponding defines from const.h.
   @see NETMD_PLAYMODE_SINGLE
   @see NETMD_PLAYMODE_REPEAT
   @see NETMD_PLAYMODE_SHUFFLE

*/
netmd_error netmd_set_playmode(netmd_dev_handle* dev, const uint16_t playmode);

/**
   Jump to the given track.

   @param dev Handle to the open minidisc player.
   @param track Number of the track to jump to.
*/
netmd_error netmd_set_track(netmd_dev_handle* dev, const uint16_t track);

/**
   Jump to the next track. If you currently playing the last track, nothing
   happens.

   @param dev Handle to the open minidisc player.
*/
netmd_error netmd_track_next(netmd_dev_handle* dev);

/**
   Jump to the previous track. If you currently playing the first track, nothing
   happens.

   @param dev Handle to the open minidisc player.
*/
netmd_error netmd_track_previous(netmd_dev_handle* dev);

/**
   Jump to the beginning of the current track.

   @param dev Handle to the open minidisc player.
*/
netmd_error netmd_track_restart(netmd_dev_handle* dev);

/**
   Jump to a specific time of the given track.

   @param dev Handle to the open minidisc player.
   @param track Track, where to jump to the given time.
   @param time Time to jump to.
*/
netmd_error netmd_set_time(netmd_dev_handle* dev, const uint16_t track,
                           const netmd_time* time);

/**
   Gets the currently playing track.

   @param dev Handle to the open minidisc player.
   @param track Pointer where to save the current track.
*/
netmd_error netmd_get_track(netmd_dev_handle* dev, uint16_t *track);

/**
   Gets the position within the currently playing track

   @param dev Handle to the open minidisc player.
   @param time Pointer to save the current time to.
*/
netmd_error netmd_get_position(netmd_dev_handle* dev, netmd_time* time);

/**
   Gets the used, total and available disc capacity (total and available
   capacity depend on current recording settings)

   @param dev Handle to the open minidisc player.
   @param capacity Pointer to a netmd_disc_capacity structure to save the
                   capacity information of the current minidisc to.
*/
netmd_error netmd_get_disc_capacity(netmd_dev_handle* dev,
                                    netmd_disc_capacity* capacity);

#endif
