/*
 * error.c
 *
 * This file is part of libnetmd, a library for accessing Sony NetMD devices.
 *
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

#include "error.h"

struct error_description {
    netmd_error error;
    const char* const description;
};

static struct error_description const descriptions[] = {
    {NETMD_NO_ERROR, "No error"},
    {NETMD_NOT_IMPLEMENTED, "Not implemented"},

    {NETMD_TRACK_DOES_NOT_EXIST, "Track does not exist"},
    {NETMD_UNSUPPORTED_FEATURE, "Device does not support this feature"},

    {NETMD_USB_OPEN_ERROR, "Error while opening the USB device"},
    {NETMD_USB_ERROR, "Generic USB error"},

    {NETMD_RESPONSE_TO_SHORT, "Response from device is shorter than expected."},
    {NETMD_RESPONSE_NOT_EXPECTED, "Response from device does not match with the expected result."},

    {NETMD_DES_ERROR, "Error during des caluclation."}
};

static char const unknown_error[] = "Unknown Error";

const char* netmd_strerror(netmd_error error)
{
    unsigned int i = 0;
    unsigned int elements;

    /* calculate the number of elements in the array */
    elements = sizeof(descriptions) / sizeof(*descriptions);

    for (i = 0; i < elements; i++) {
        if (descriptions[i].error == error) {
            return descriptions[i].description;
        }
    }

    return unknown_error;
}
