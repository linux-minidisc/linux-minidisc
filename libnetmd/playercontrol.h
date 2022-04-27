#ifndef LIBNETMD_PLAYERCONTROL_H
#define LIBNETMD_PLAYERCONTROL_H

/** \file playercontrol.h */

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
 * Double a netmd_time (for SP/LP2/LP4 duration)
 *
 * This is useful for translating SP to Mono/LP2 or LP2 to LP4 time.
 * At the moment, the netmd_time.frame value is ignored / set to zero.
 *
 * @param time The value to be multipled by 2 (in-place)
 */
void netmd_time_double(netmd_time *time);

/**
 * Calculate number of seconds in a netmd_time structure
 *
 * @param time The value to be converted
 * @return The duration in seconds (frames are ignored)
 */
int netmd_time_to_seconds(const netmd_time *time);

/**
   Structure to hold the capacity information of a disc.
*/
typedef struct {
    /** Time already recorded on the disc. This is the sum of the track
     * durations independent of recording mode (SP/Mono/LP2/LP4), and
     * therefore might not be the same as (total - available).
     *
     * To calculate the amount of time already recorded in SP units,
     * subtract the available time from the total time.
     **/
    netmd_time recorded;

    /** Total time that can be recorded on the disc in SP units, usually:
     * 60:59 .. for 60-minute MDs
     * 74:59 .. for 74-minute MDs
     * 80:59 .. for 80-minute MDs
     **/
    netmd_time total;

    /** Time that is available on the disc in SP units. */
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
netmd_error netmd_set_current_track(netmd_dev_handle* dev, netmd_track_index track);

/**
   Gets the currently playing track.

   @param dev Handle to the open minidisc player.
   @return The current track, or NETMD_INVALID_TRACK on error.
*/
netmd_track_index netmd_get_current_track(netmd_dev_handle* dev);


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
netmd_error netmd_set_time(netmd_dev_handle* dev, netmd_track_index track,
                           const netmd_time* time);

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
