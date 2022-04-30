#ifndef LIBNETMD_TRACKINFORMATION_H
#define LIBNETMD_TRACKINFORMATION_H

/** \file trackinformation.h */

#include <stdint.h>

#include "common.h"
#include "const.h"
#include "error.h"

/**
 * Struct filled in by netmd_get_track_info(), see there.
 */
struct netmd_track_info {
    netmd_track_index track_id; /*!< 0-based track index on the disc */

    const char *title; /*!< User-visible track title, with any "LP:" prefix stripped for LP2/LP4 tracks */
    netmd_time duration; /*!< Duration of the track (in wall time) */
    enum NetMDEncoding encoding; /*!< Encoding used (SP, LP2, LP4) */
    enum NetMDChannels channels; /*!< For SP tracks, whether it is encoded as Mono or Stereo */
    enum NetMDTrackFlags protection; /*!< DRM status of the track (for upload and deletion) */

    char raw_title[256]; /*!< Storage for the actual title (including any "LP:" prefixes) as present on disk */
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
netmd_error
netmd_get_track_info(netmd_dev_handle *dev, netmd_track_index track_id, struct netmd_track_info *info);

/**
   Get the encoding used to encode a specific track.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param encoding Pointer to store the codec
   @param channel Pointer to store the channel mode
   @return true on success, false on error
*/
bool
netmd_request_track_encoding(netmd_dev_handle *dev, netmd_track_index track,
        unsigned char *encoding, unsigned char *channel);

/**
   Get the flags used for a specific track.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param data pointer to store the flags
*/
bool
netmd_request_track_flags(netmd_dev_handle *dev, netmd_track_index track, unsigned char *data);

/**
   Get the title for a specific track.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer buffer to hold the name.
   @param size of buf.
   @return Actual size of buffer, if your buffer is too small resize buffer and
           recall function.
*/
int netmd_request_title(netmd_dev_handle* dev, netmd_track_index track, char* buffer, const size_t size);

bool
netmd_request_track_time(netmd_dev_handle* dev, netmd_track_index track, netmd_time *time);

#endif
