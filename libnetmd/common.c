/* common.c
 *      Copyright (C) 2004, Bertrik Sikken
 *      Copyright (C) 2011, Alexander Sulfrian
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

#include "common.h"
#include "const.h"
#include "trace.h"

#define NETMD_POLL_TIMEOUT 1000	/* miliseconds */
#define NETMD_SEND_TIMEOUT 1000
#define NETMD_RECV_TIMEOUT 1000
#define NETMD_RECV_TRIES 30

/*
  polls to see if minidisc wants to send data

  @param dev USB device handle
  @param buf pointer to poll buffer
  @param tries maximum attempts to poll the minidisc
  @return if error <0, else number of bytes that md wants to send
*/
static int netmd_poll(usb_dev_handle *dev, char *buf, int tries)
{
    int i;

    for (i = 0; i < tries; i++) {
        /* send a poll message */
        memset(buf, 0, 4);

        if (usb_control_msg(dev, USB_ENDPOINT_IN | USB_TYPE_VENDOR |
                            USB_RECIP_INTERFACE, 0x01, 0, 0, buf, 4,
                            NETMD_POLL_TIMEOUT) < 0) {
            netmd_trace(NETMD_TRACE_ERROR, "netmd_poll: usb_control_msg failed\n");
            return NETMDERR_USB;
        }

        if (buf[0] != 0) {
            break;
        }

        if (i > 0) {
            sleep(1);
        }
    }

    return buf[2];
}


int netmd_exch_message(netmd_dev_handle *devh, unsigned char *cmd,
                       const size_t cmdlen, unsigned char *rsp)
{
    char pollbuf[4];
    unsigned char rsp_code;
    int	len;
    usb_dev_handle *dev;

    dev = (usb_dev_handle *)devh;

    /* poll to see if we can send data */
    len = netmd_poll(dev, pollbuf, 1);
    if (len != 0) {
        netmd_trace(NETMD_TRACE_ERROR, "netmd_exch_message: netmd_poll failed\n");
        return (len > 0) ? NETMDERR_NOTREADY : len;
    }

    /* send data */
    netmd_trace(NETMD_TRACE_INFO, "Command:\n");
    netmd_trace_hex(NETMD_TRACE_INFO, cmd, cmdlen);
    if (usb_control_msg(dev, USB_ENDPOINT_OUT | USB_TYPE_VENDOR |
                        USB_RECIP_INTERFACE, 0x80, 0, 0, (char*)cmd, (int)cmdlen,
                        NETMD_SEND_TIMEOUT) < 0) {
        netmd_trace(NETMD_TRACE_ERROR, "netmd_exch_message: usb_control_msg failed\n");
        return NETMDERR_USB;
    }

    do {
        /* poll for data that minidisc wants to send */
        len = netmd_poll(dev, pollbuf, NETMD_RECV_TRIES);
        if (len <= 0) {
            netmd_trace(NETMD_TRACE_ERROR, "netmd_exch_message: netmd_poll failed\n");
            return (len == 0) ? NETMDERR_TIMEOUT : len;
        }

        /* receive data */
        if (usb_control_msg(dev, USB_ENDPOINT_IN | USB_TYPE_VENDOR |
                            USB_RECIP_INTERFACE, pollbuf[1], 0, 0, (char*)rsp, len,
                            NETMD_RECV_TIMEOUT) < 0) {
            netmd_trace(NETMD_TRACE_ERROR, "netmd_exch_message: usb_control_msg failed\n");
            return NETMDERR_USB;
        }

        netmd_trace(NETMD_TRACE_INFO, "Response:\n");
        netmd_trace_hex(NETMD_TRACE_INFO, rsp, len);

        rsp_code = rsp[0];
        switch (rsp_code) {
        case 0x0f:	netmd_trace(NETMD_TRACE_INFO, "Command acknowledged\n"); break;
        case 0x0c:	netmd_trace(NETMD_TRACE_INFO, "** Unknown Header\n"); break;
        case 0x09:	netmd_trace(NETMD_TRACE_INFO, "Command successful\n"); break;
        case 0x08:	netmd_trace(NETMD_TRACE_INFO, "** Unknown Command\n"); break;
        case 0x0a:	netmd_trace(NETMD_TRACE_INFO, "** Error on record\n"); break;
        default: 	netmd_trace(NETMD_TRACE_INFO, "** Unknown return code\n"); break;
        }

        /* get response again if player responds with 0x0F.	*/
    } while (rsp_code == 0x0F);

    /* return length */
    return len;
}
