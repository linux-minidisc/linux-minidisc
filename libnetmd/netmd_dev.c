/* netmd_dev.c
 *      Copyright (C) 2004, Bertrik Sikken
 *      Copyright (C) 2011, Adrian Glaubitz
 *
 * This file is part of libnetmd.
 *
 * libnetmd is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Libnetmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include <libusb.h>

#include "netmd_dev.h"
#include "libusbmd.h"
#include "groups.h"

#include "log.h"
#include "const.h"

static libusb_context *ctx = NULL;

netmd_error netmd_init(netmd_device **device_list, libusb_context *hctx)
{
    ssize_t usb_device_count;
    ssize_t i = 0;
    netmd_device *new_device;
    libusb_device **list;
    struct libusb_device_descriptor desc;

    /* skip device enumeration when using libusb hotplug feature
     * use libusb_context of running libusb instance from hotplug initialisation */
    if(hctx) {
        ctx = hctx;
        return NETMD_USE_HOTPLUG;
    }

    libusb_init(&ctx);

    *device_list = NULL;

    usb_device_count = libusb_get_device_list(ctx, &list);

    for (i = 0; i < usb_device_count; i++) {
      libusb_get_device_descriptor(list[i], &desc);

      const struct minidisc_usb_device_info *info = minidisc_usb_device_info_get(desc.idVendor, desc.idProduct);
      if (info != NULL && info->device_type == MINIDISC_USB_DEVICE_TYPE_NETMD) {
          new_device = malloc(sizeof(netmd_device));
          new_device->usb_dev = list[i];
          new_device->next = *device_list;
          new_device->device_name = info->name;
          *device_list = new_device;
      }

    }

    return NETMD_NO_ERROR;
}


netmd_error netmd_open(netmd_device *dev, netmd_dev_handle **dev_handle)
{
    int result;
    libusb_device_handle *dh = NULL;

    result = libusb_open(dev->usb_dev, &dh);
    if (result == 0)
        result = libusb_claim_interface(dh, 0);

    if (result == 0) {
        struct libusb_device_descriptor desc = {0};
        libusb_device *device = libusb_get_device(dh);
        if (libusb_get_device_descriptor(device, &desc) != 0) {
            return NETMD_USB_OPEN_ERROR;
        }

        *dev_handle = malloc(sizeof(netmd_dev_handle));
        memset(*dev_handle, 0, sizeof(**dev_handle));

        (*dev_handle)->usb = dh;
        (*dev_handle)->vendor_id = desc.idVendor;
        (*dev_handle)->product_id = desc.idProduct;
        return NETMD_NO_ERROR;
    }
    else {
        *dev_handle = NULL;
        return NETMD_USB_OPEN_ERROR;
    }
}

bool netmd_dev_can_upload(netmd_dev_handle *devh)
{
    return devh->vendor_id == NETMD_VENDOR_ID_SONY && devh->product_id == NETMD_PRODUCT_ID_MZ_RH1;
}

netmd_error netmd_close(netmd_dev_handle* devh)
{
    if (libusb_release_interface(devh->usb, 0) == 0) {
        libusb_close(devh->usb);
        netmd_clear_disc_header(devh);
        free(devh);
    } else {
        return NETMD_USB_ERROR;
    }

    return NETMD_NO_ERROR;
}


void netmd_clean(netmd_device **device_list)
{
    netmd_device *tmp, *device;

    device = *device_list;
    while (device != NULL) {
        tmp = device->next;
        free(device);
        device = tmp;
    }

    *device_list = NULL;

    libusb_exit(ctx);
}
