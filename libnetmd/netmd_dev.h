/* netmd_dev.h
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

#ifndef _NETMD_DEV_H_
#define _NETMD_DEV_H_

#include <usb.h>

/** Error codes of the USB transport layer */
#define NETMDERR_USB					-1	/* general USB error */
#define NETMDERR_NOTREADY			-2	/* player not ready for command */
#define NETMDERR_TIMEOUT			-3	/* timeout while waiting for response */
#define NETMDERR_CMD_FAILED		-4	/* minidisc responded with 08 response */
#define NETMDERR_CMD_INVALID	-5	/* minidisc responded with 0A response */



typedef struct netmd_device_t {
	struct netmd_device_t	*link;
	char			name[32];
	struct usb_device	*usb_dev;
} netmd_device_t;

typedef usb_dev_handle*		*netmd_dev_handle;


/** Struct to hold the vendor and product id's for each unit. */
struct netmd_devices {
	int	idVendor;
	int	idProduct;
};

/** Finds netmd device and returns pointer to its device handle */
int netmd_init(netmd_device_t **device_list);

/*! Initialize USB subsystem for talking to NetMD
  \param dev Pointer returned by netmd_init.
*/
netmd_dev_handle* netmd_open(netmd_device_t *netmd_dev);

/*! Get the device name stored in USB device
  \param dev pointer to device returned by netmd_open
  \param buf buffer to hold the name.
  \param buffsize of buf.
  \return Actual size of buffer, if your buffer is too small resize buffer and recall function.
*/
int netmd_get_devname(netmd_dev_handle* dh, unsigned char* buf, int buffsize);

/*! Function for internal use by init_disc_info */
// int request_disc_title(usb_dev_handle* dev, char* buffer, int size);

/*! Function to exchange command/response buffer with minidisc
	\param dev device handle
	\param cmd command buffer
	\param cmdlen length of command
	\param rsp response buffer
	\return number of bytes received if >0, or error if <0
*/
int netmd_exch_message(netmd_dev_handle *dev, unsigned char *cmd, int cmdlen,
	unsigned char *rsp);

/*! closes the usb descriptors
  \param dev pointer to device returned by netmd_open
*/
void netmd_clean(netmd_dev_handle* dev);

#endif /* _NETMD_DEV_H_ */
