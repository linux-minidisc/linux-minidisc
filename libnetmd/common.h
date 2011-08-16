#ifndef LIBNETMD_COMMON_H
#define LIBNETMD_COMMON_H

#include <usb.h>

typedef usb_dev_handle*	netmd_dev_handle;

/**
  Function to exchange command/response buffer with minidisc

  @param dev device handle
  @param cmd command buffer
  @param cmdlen length of command
  @param rsp response buffer
  @return number of bytes received if >0, or error if <0
*/
int netmd_exch_message(netmd_dev_handle *dev, unsigned char *cmd,
                       const size_t cmdlen, unsigned char *rsp);

#endif
