#pragma once

/**
 * libusbmd: NetMD/HiMD USB Detection Library
 * Copyright (c) 2022, Thomas Perl <m@thp.io>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **/

#ifdef __cplusplus
extern "C" {
#endif

enum MinidiscUSBDeviceType {
    MINIDISC_USB_DEVICE_TYPE_NETMD = 0,
    MINIDISC_USB_DEVICE_TYPE_HIMD = 1,
};

struct minidisc_usb_device_info {
    enum MinidiscUSBDeviceType device_type;
    int vendor_id;
    int product_id;
    const char *name;
};

/**
 * Look up device information based on USB vendor ID and product ID.
 *
 * @param vendor_id The USB Vendor ID of the USB device
 * @param product_id The USB Product ID of the USB device
 * @return Pointer to the matching entry, or NULL if not found.
 */
const struct minidisc_usb_device_info *
minidisc_usb_device_info_get(int vendor_id, int product_id);

/**
 * Get the first entry in the known USB device info list.
 *
 * This returns the first entry.
 *
 * @return A pointer to the first entry in the list
 */
const struct minidisc_usb_device_info *
minidisc_usb_device_info_first(void);

/**
 * Get the next entry in the known USB device info list.
 *
 * @param info The current entry in the device info list
 * @return Pointer to the next entry, or NULL if there are no more entries
 */
const struct minidisc_usb_device_info *
minidisc_usb_device_info_next(const struct minidisc_usb_device_info *info);

#ifdef __cplusplus
}
#endif
