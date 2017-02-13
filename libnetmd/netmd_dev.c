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

#include "netmd_dev.h"
#include "log.h"
#include "const.h"

static libusb_context *ctx = NULL;

/*! list of known vendor/prod id's for NetMD devices */
static struct netmd_devices const known_devices[] =
{
    {0x54c, 0x34}, /* Sony PCLK-XX */
    {0x54c, 0x36}, /* Sony (unknown model) */
    {0x54c, 0x75}, /* Sony MZ-N1 */
    {0x54c, 0x7c}, /* Sony (unknown model) */
    {0x54c, 0x80}, /* Sony LAM-1 */
    {0x54c, 0x81}, /* Sony MDS-JE780/JB980 */
    {0x54c, 0x84}, /* Sony MZ-N505 */
    {0x54c, 0x85}, /* Sony MZ-S1 */
    {0x54c, 0x86}, /* Sony MZ-N707 */
    {0x54c, 0x8e}, /* Sony CMT-C7NT */
    {0x54c, 0x97}, /* Sony PCGA-MDN1 */
    {0x54c, 0xad}, /* Sony CMT-L7HD */
    {0x54c, 0xc6}, /* Sony MZ-N10 */
    {0x54c, 0xc7}, /* Sony MZ-N910 */
    {0x54c, 0xc8}, /* Sony MZ-N710/NE810/NF810 */
    {0x54c, 0xc9}, /* Sony MZ-N510/NF610 */
    {0x54c, 0xca}, /* Sony MZ-NE410/DN430/NF520 */
    {0x54c, 0xeb}, /* Sony MZ-NE810/NE910 */
    {0x54c, 0xe7}, /* Sony CMT-M333NT/M373NT */
    {0x54c, 0x101}, /* Sony LAM-10 */
    {0x54c, 0x113}, /* Aiwa AM-NX1 */
    {0x54c, 0x14c}, /* Aiwa AM-NX9 */
    {0x54c, 0x17e}, /* Sony MZ-NH1 */
    {0x54c, 0x180}, /* Sony MZ-NH3D */
    {0x54c, 0x182}, /* Sony MZ-NH900 */
    {0x54c, 0x184}, /* Sony MZ-NH700/800 */
    {0x54c, 0x186}, /* Sony MZ-NH600/600D */
    {0x54c, 0x188}, /* Sony MZ-N920 */
    {0x54c, 0x18a}, /* Sony LAM-3 */
    {0x54c, 0x1e9}, /* Sony MZ-DH10P */
    {0x54c, 0x219}, /* Sony MZ-RH10 */
    {0x54c, 0x21b}, /* Sony MZ-RH910 */
    {0x54c, 0x21d}, /* Sony CMT-AH10 */
    {0x54c, 0x22c}, /* Sony CMT-AH10 */
    {0x54c, 0x23c}, /* Sony DS-HMD1 */
    {0x54c, 0x286}, /* Sony MZ-RH1 */

    {0x4dd, 0x7202}, /* Sharp IM-MT880H/MT899H */
    {0x4dd, 0x9013}, /* Sharp IM-DR400/DR410 */
    {0x4dd, 0x9014}, /* Sharp IM-DR80/DR420/DR580 and Kenwood DMC-S9NET */

    {0, 0} /* terminating pair */
};


netmd_error netmd_init(netmd_device **device_list, libusb_context *hctx)
{
    int count = 0;
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

      for (count = 0; known_devices[count].idVendor != 0 && known_devices[count].idProduct != 0; count++) {
	if(desc.idVendor == known_devices[count].idVendor &&
	   desc.idProduct == known_devices[count].idProduct) {
	  new_device = malloc(sizeof(netmd_device));
	  new_device->usb_dev = list[i];
	  new_device->link = *device_list;
	  *device_list = new_device;
	}
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
        *dev_handle = (netmd_dev_handle*)dh;
        return NETMD_NO_ERROR;
    }
    else {
        *dev_handle = NULL;
        return NETMD_USB_OPEN_ERROR;
    }
}

netmd_error netmd_get_devname(netmd_dev_handle* devh, char *buf, size_t buffsize)
{
    int result;

    result = libusb_get_string_descriptor_ascii((libusb_device_handle *)devh, 2, (unsigned char *)buf, buffsize);
    if (result < 0) {
        netmd_log(NETMD_LOG_ERROR, "libusb_get_string_descriptor_asci failed, %s (%d)\n", strerror(errno), errno);
        buf[0] = 0;
        return NETMD_USB_ERROR;
    }

    return NETMD_NO_ERROR;
}

netmd_error netmd_close(netmd_dev_handle* devh)
{
    int result;
    libusb_device_handle *dev;

    dev = (libusb_device_handle *)devh;
    result = libusb_release_interface(dev, 0);
    if (result == 0)
      libusb_close(dev);
    else{
        return NETMD_USB_ERROR;
    }

    return NETMD_NO_ERROR;
}


void netmd_clean(netmd_device **device_list)
{
    netmd_device *tmp, *device;

    device = *device_list;
    while (device != NULL) {
        tmp = device->link;
        free(device);
        device = tmp;
    }

    *device_list = NULL;

    libusb_exit(ctx);
}
