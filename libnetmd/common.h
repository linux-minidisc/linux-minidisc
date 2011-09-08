#ifndef LIBNETMD_COMMON_H
#define LIBNETMD_COMMON_H

#include <libusb.h>

/**
   Typedef that nearly all netmd_* functions use to identify the USB connection
   with the minidisc player.
*/
typedef libusb_device_handle *netmd_dev_handle;

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


#endif
