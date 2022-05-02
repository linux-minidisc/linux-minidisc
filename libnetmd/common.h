#ifndef LIBNETMD_COMMON_H
#define LIBNETMD_COMMON_H

/** \file common.h */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * A zero-based track index.
 */
typedef uint16_t netmd_track_index;

/**
 * Invalid track ID, returned by some functions.
 */
#define NETMD_INVALID_TRACK ((netmd_track_index)-1)


/**
 * Group index (1-based).
 */
typedef int netmd_group_id;


/**
 * Time duration value.
 */
typedef struct netmd_time {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t frame; // "Frames per second = 86 for MD audio", a "sound group" (AV/C MD Audio 1.0)
} netmd_time;

/**
 * 8-byte track UUID
 */
typedef struct netmd_uuid {
    uint8_t uuid[8];
} netmd_uuid;

/**
 * Forward-declaration of opaque libusb structures
 */
typedef struct libusb_context libusb_context;
typedef struct libusb_device libusb_device;

/**
   Data about a group, start track, finish track and name. Used to generate disc
   header info.
*/
struct netmd_group {
    int first; /*!< First track index, 1-based; 0 for disc title and empty groups */
    int last; /*!< First track index, 1-based, 0 for disc title and empty groups */
    char *name; /*!< Group name */
};

/**
 * Group data and disc title.
 */
struct netmd_groups {
    bool valid; /*!< true if the group data has been loaded, false otherwise */
    struct netmd_group *groups; /*!< Array of groups (index zero = disc title) */
    size_t count; /*!< Number of groups (at least 1, the disc title) */
};

/**
   Typedef that nearly all netmd_* functions use to identify the USB connection
   with the minidisc player.
*/

typedef struct netmd_dev_handle {
    struct libusb_device_handle *usb; /*!< USB device handle */
    struct netmd_groups groups; /*!< Parsed group information (disc title) */
} netmd_dev_handle;

/**
  Function to exchange command/response buffer with minidisc player.

  @param dev device handle
  @param cmd buffer with the command, that should be send to the player
  @param cmdlen length of the command
  @param rsp buffer where the response should be written to
  @return number of bytes received if >0, or error if <0
*/
int netmd_exch_message(netmd_dev_handle *dev, unsigned char *cmd,
                       const size_t cmdlen, unsigned char *rsp);

/**
  Function to send a command to the minidisc player.

  @param dev device handle
  @param cmd buffer with the command, that should be send to the player
  @param cmdlen length of the command
*/
int netmd_send_message(netmd_dev_handle *dev, unsigned char *cmd,
                       const size_t cmdlen);


/**
  Function to recieve a response from the minidisc player.

  @param rsp buffer where the response should be written to
  @return number of bytes received if >0, or error if <0
*/
int netmd_recv_message(netmd_dev_handle *dev, unsigned char *rsp);

/**
   Wait for the device to respond to commands. Should only be used
   when the device needs to be given "breathing room" and is not
   expected to have anything to send.

   @param dev device handle
   @return 1 if success, 0 if there was no response
*/
int netmd_wait_for_sync(netmd_dev_handle* dev);

#endif
