#ifndef LIBNETMD_DEV_H
#define LIBNETMD_DEV_H

#include "error.h"
#include "common.h"

typedef struct netmd_device {
    struct netmd_device *link;
    char name[32];
    struct libusb_device *usb_dev;
} netmd_device;

/**
  Struct to hold the vendor and product id's for each unit.
*/
struct netmd_devices {
    int	idVendor;
    int	idProduct;
};

/**
  Intialises the netmd device layer, scans the USB and fills in a list of
  supported devices.

  @param device_list Linked list of netmd_device_t structures to fill.
  @param libusb_context of a running instance of libusb
*/
netmd_error netmd_init(netmd_device **device_list, libusb_context * hctx);

/**
  Opens a NetMD device.

  @param dev Pointer to a device discoverd by netmd_init.
  @param dev_handle Pointer to variable to save the handle of the opened
                    device used for communication in all other netmd_
                    functions.
*/
netmd_error netmd_open(netmd_device *dev, netmd_dev_handle **dev_handle);

/**
  Get the device name stored in USB device.

  @param devh Pointer to device, returned by netmd_open.
  @param buf Buffer to hold the name.
  @param buffsize Available size in buf.
*/
netmd_error netmd_get_devname(netmd_dev_handle* devh, char *buf, size_t buffsize);

/**
  Closes the usb descriptors.

  @param dev Pointer to device returned by netmd_open.
*/
netmd_error netmd_close(netmd_dev_handle* dev);

/**
  Cleans structures created by netmd_init.

  @param device_list List of devices filled by netmd_init.
*/
void netmd_clean(netmd_device **device_list);


#endif
