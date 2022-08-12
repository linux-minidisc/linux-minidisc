#ifndef LIBNETMD_TRACKINFORMATION_H
#define LIBNETMD_TRACKINFORMATION_H

#include <stdint.h>
#include <stdbool.h>

#include "common.h"

/**
   Get the bitrate used to encode a specific track.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param data pointer to store the hex code representing the bitrate.
*/
int netmd_request_track_bitrate(netmd_dev_handle*dev, const uint16_t track,
                                unsigned char* encoding, unsigned char* channel);

/**
   Get the flags used for a specific track.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param data pointer to store the hex code representing the codec.
*/
int netmd_request_track_flags(netmd_dev_handle* dev, const uint16_t track, unsigned char* data);

/**
   Get the title for a specific track.

   Titles retrieved using this function are converted to
   UTF-8 for your convenience.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer buffer to hold the name.
   @param size of buf.
   @return Actual size of buffer, if your buffer is too small resize buffer and
           recall function.
*/
int netmd_request_title(netmd_dev_handle* dev, const uint16_t track, char* buffer, const size_t size);

/**
   Get the raw title for a specific track.

   Narrow titles will be returned as raw JIS X0201 text,
   wide titles will be returned as raw Shift JIS text.
   A given MiniDisc track may only have one set.

   For automatic detection, and UTF-8 conversion,
   please use netmd_request_title instead.

   @param dev pointer to device returned by netmd_open
   @param track Zero based index of track your requesting.
   @param buffer buffer to hold the name.
   @param size of buf.
   @param whether to request the narrow or wide title.
   @return Actual size of buffer, if your buffer is too small resize buffer and
           recall function.
*/
int netmd_request_title_raw(netmd_dev_handle* dev, const uint16_t track, char* buffer, const size_t size, bool wide_chars);

#endif
