/*
 * common.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
 * Copyright (C) 2004 Bertrik Sikken
 * Copyright (C) 2011 Alexander Sulfrian
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <string.h>
#include <unistd.h>

#include "common.h"

#include "const.h"
#include "log.h"
#include "utils.h"

#include <libusb.h>

#define NETMD_POLL_TIMEOUT 1000	/* miliseconds */
#define NETMD_SEND_TIMEOUT 1000
#define NETMD_RECV_TIMEOUT 1000
#define NETMD_RECV_TRIES 30
#define NETMD_SYNC_TRIES 5

/*
  polls to see if minidisc wants to send data

  @param dev USB device handle
  @param buf pointer to poll buffer
  @param tries maximum attempts to poll the minidisc
  @return if error <0, else number of bytes that md wants to send
*/
static int netmd_poll(libusb_device_handle *dev, unsigned char *buf, int tries)
{
    /* original netmd poll sleep time was 1s, which lead to a print disc info
       taking ~50s on a JE780. Dropping down to 5ms dropped print disc info
       time to 0.54s, but was hitting timeout limits when sending tracks.
       Unsure if just increasing limit is good practice, so set sleep time to
       grow back to 1s if it retries more than 10x. Testing with 780 shows this
       works for track transfers, typically hitting 15 loop iterations.
    */
    int i;
    int sleepytime = 5;

    for (i = 0; i < tries; i++) {
        /* send a poll message */
        memset(buf, 0, 4);

        if (libusb_control_transfer(dev, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                            LIBUSB_RECIPIENT_INTERFACE, 0x01, 0, 0, buf, 4,
                            NETMD_POLL_TIMEOUT) < 0) {
            netmd_log(NETMD_LOG_ERROR, "netmd_poll: libusb_control_transfer failed\n");
            return NETMDERR_USB;
        }

        if (buf[0] != 0) {
            break;
        }

        if (i > 0) {
            msleep(sleepytime);
            sleepytime = 100;
        }
        if (i > 10) {
          sleepytime = 1000;
        }
    }

    return buf[2];
}


static const char *
netmd_response_code_name(uint8_t code)
{
    switch (code) {
        // AVC Digital Interface Commands 3.0, "5.3.2 Response code"
        case 0x8: return "NOT IMPLEMENTED";
        case 0x9: return "ACCEPTED";
        case 0xA: return "REJECTED";
        case 0xB: return "IN TRANSITION";
        case 0xC: return "IMPLEMENTED / STABLE";
        case 0xD: return "CHANGED";
        case 0xE: return "Reserved";
        case 0xF: return "INTERIM";
        default: return "unknown";
    }
}


int netmd_exch_message(netmd_dev_handle *devh, unsigned char *cmd,
                       const size_t cmdlen, unsigned char *rsp)
{
    int len;
    netmd_send_message(devh, cmd, cmdlen);
    len = netmd_recv_message(devh, rsp);
    netmd_log(NETMD_LOG_DEBUG, "Response code: %d [%s]\n", rsp[0], netmd_response_code_name(rsp[0]));
    netmd_log_hex(NETMD_LOG_DEBUG, &rsp[0], 1);
    if (rsp[0] == NETMD_STATUS_INTERIM) {
      netmd_log(NETMD_LOG_DEBUG, "Re-reading:\n");
      len = netmd_recv_message(devh, rsp);
      netmd_log(NETMD_LOG_DEBUG, "Response code: %s [%s]\n", rsp[0], netmd_response_code_name(rsp[0]));
      netmd_log_hex(NETMD_LOG_DEBUG, &rsp[0], 1);
    }
    return len;
}


int netmd_send_message(netmd_dev_handle *devh, unsigned char *cmd,
                       const size_t cmdlen)
{
    unsigned char pollbuf[4];
    int	len;
    libusb_device_handle *dev = devh->usb;

    /* poll to see if we can send data */
    len = netmd_poll(dev, pollbuf, 1);
    if (len != 0) {
        netmd_log(NETMD_LOG_ERROR, "netmd_exch_message: netmd_poll failed\n");
        return (len > 0) ? NETMDERR_NOTREADY : len;
    }

    /* send data */
    netmd_log(NETMD_LOG_DEBUG, "Command:\n");
    netmd_log_hex(NETMD_LOG_DEBUG, cmd, cmdlen);
    if (libusb_control_transfer(dev, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
                        LIBUSB_RECIPIENT_INTERFACE, 0x80, 0, 0, cmd, (int)cmdlen,
                        NETMD_SEND_TIMEOUT) < 0) {
        netmd_log(NETMD_LOG_ERROR, "netmd_exch_message: libusb_control_transfer failed\n");
        return NETMDERR_USB;
    }

    return 0;
}

int netmd_recv_message(netmd_dev_handle *devh, unsigned char* rsp)
{
    int len;
    unsigned char pollbuf[4];
    libusb_device_handle *dev = devh->usb;

    /* poll for data that minidisc wants to send */
    len = netmd_poll(dev, pollbuf, NETMD_RECV_TRIES);
    if (len <= 0) {
        netmd_log(NETMD_LOG_ERROR, "netmd_exch_message: netmd_poll failed\n");
        return (len == 0) ? NETMDERR_TIMEOUT : len;
    }

    /* receive data */
    if (libusb_control_transfer(dev, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR |
                        LIBUSB_RECIPIENT_INTERFACE, pollbuf[1], 0, 0, rsp, len,
                        NETMD_RECV_TIMEOUT) < 0) {
        netmd_log(NETMD_LOG_ERROR, "netmd_exch_message: libusb_control_transfer failed\n");
        return NETMDERR_USB;
    }

    netmd_log(NETMD_LOG_DEBUG, "Response:\n");
    netmd_log_hex(NETMD_LOG_DEBUG, rsp, (size_t)len);

    /* return length */
    return len;
}

/* Wait for the device to respond to a command (any command). Some
 * devices need to be given a bit of "breathing room" to avoid USB
 * interface crashes, which is what this does.
 *
 * This is almost the same as netmd_poll, they could probably be
 * merged at some point. */
int netmd_wait_for_sync(netmd_dev_handle* devh)
{
    unsigned char syncmsg[4];
    int tries = NETMD_SYNC_TRIES;
    libusb_device_handle *dev = devh->usb;
    int ret;

    do {
        ret = libusb_control_transfer(dev, LIBUSB_ENDPOINT_IN |
                                      LIBUSB_REQUEST_TYPE_VENDOR |
                                      LIBUSB_RECIPIENT_INTERFACE,
                                      0x01, 0, 0,
                                      syncmsg, 0x04,
                                      NETMD_POLL_TIMEOUT * 5);
        tries -= 1;
        if (ret < 0) {
            netmd_log(NETMD_LOG_VERBOSE, "netmd_wait_for_sync: libusb error %d waiting for control transfer\n", ret);
        } else if (ret != 4) {
            netmd_log(NETMD_LOG_VERBOSE, "netmd_wait_for_sync: control transfer returned %d bytes instead of the expected 4\n", ret);
        } else if (memcmp(syncmsg, "\0\0\0\0", 4) == 0) {
            /* When the device returns 00 00 00 00 we are done. */
            break;
        }
        
        msleep(100);
    } while (tries);

    if (tries == 0)
        netmd_log(NETMD_LOG_WARNING, "netmd_wait_for_sync: no sync response from device\n"); 
    else if (tries != (NETMD_SYNC_TRIES - 1))
        /* Notify if we ended up waiting for more than one iteration */
        netmd_log(NETMD_LOG_VERBOSE, "netmd_wait_for_sync: waited for sync, %d tries remained\n", tries);

    return (tries > 0);
}
