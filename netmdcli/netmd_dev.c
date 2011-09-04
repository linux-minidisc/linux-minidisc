/* netmd_dev.c
 *      Copyright (C) 2004, Bertrik Sikken
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

#include "netmd_dev.h"
#include "log.h"
#include "const.h"

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


netmd_error netmd_init(netmd_device **device_list)
{
    struct usb_bus *bus;
    struct usb_device *dev;
    int count = 0;
    netmd_device *new_device;

    usb_init();

    usb_find_busses();
    usb_find_devices();

    *device_list = NULL;
    for(bus = usb_get_busses(); bus; bus = bus->next)
    {
        for(dev = bus->devices; dev; dev = dev->next)
        {
            for(count = 0; (known_devices[count].idVendor != 0); count++)
            {
                if(dev->descriptor.idVendor == known_devices[count].idVendor
                   && dev->descriptor.idProduct == known_devices[count].idProduct)
                {
                    new_device = malloc(sizeof(netmd_device));
                    new_device->usb_dev = dev;
                    new_device->link = *device_list;
                    *device_list = new_device;
                }
            }
        }
    }

    return NETMD_NO_ERROR;
}


netmd_error netmd_open(netmd_device *dev, netmd_dev_handle **dev_handle)
{
    int result;
    usb_dev_handle *dh;

    dh = usb_open(dev->usb_dev);
    result = usb_claim_interface(dh, 0);

    if (result == 0) {
        *dev_handle = (netmd_dev_handle*)dh;
        return NETMD_NO_ERROR;
    }
    else {
        *dev_handle = NULL;
        return NETMD_USB_OPEN_ERROR;
    }
}

netmd_error netmd_get_devname(netmd_dev_handle* devh, char* buf, size_t buffsize)
{
    int result;

    result = usb_get_string_simple((usb_dev_handle *)devh, 2, buf, buffsize);
    if (result < 0) {
        netmd_log(NETMD_LOG_ERROR, "usb_get_string_simple failed, %s (%d)\n", strerror(errno), errno);
        buf[0] = 0;
        return NETMD_USB_ERROR;
    }

    return NETMD_NO_ERROR;
}

netmd_error netmd_close(netmd_dev_handle* devh)
{
    int result;
    usb_dev_handle *dev;

    dev = (usb_dev_handle *)devh;
    result = usb_release_interface(dev, 0);
    if (result == 0) {
        result = usb_close(dev);
    }

    if (result < 0) {
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
}
