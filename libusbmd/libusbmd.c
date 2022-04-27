/*
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
 */

#include "libusbmd.h"

#include "libusbmd_data.h"

const struct minidisc_usb_device_info *
minidisc_usb_device_info_get(int vendor_id, int product_id)
{
    const struct minidisc_usb_device_info *cur = minidisc_usb_device_info_first();
    while (cur != NULL) {
        if (cur->vendor_id == vendor_id && cur->product_id == product_id) {
            return cur;
        }

        cur = minidisc_usb_device_info_next(cur);
    }

    return NULL;
}

const struct minidisc_usb_device_info *
minidisc_usb_device_info_first(void)
{
    return KNOWN_DEVICES;
}

const struct minidisc_usb_device_info *
minidisc_usb_device_info_next(const struct minidisc_usb_device_info *info)
{
    if (!info) {
        return NULL;
    }

    ++info;

    if (!info->name) {
        return NULL;
    }

    return info;
}
