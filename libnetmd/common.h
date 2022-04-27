#ifndef LIBNETMD_COMMON_H
#define LIBNETMD_COMMON_H

/** \file common.h */

#include <stddef.h>

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
 * Forward-declaration of opaque libusb structures
 */
typedef struct libusb_context libusb_context;
typedef struct libusb_device libusb_device;

/**
   Typedef that nearly all netmd_* functions use to identify the USB connection
   with the minidisc player.
*/

typedef struct libusb_device_handle *netmd_dev_handle;

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
