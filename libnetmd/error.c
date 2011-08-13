/* error.c
 *      Copyright (C) 2011 Alexander Sulfrian
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

#include "error.h"

struct error_description {
    netmd_error error;
    const char* const description;
};

static struct error_description const descriptions[] = {
    {NETMD_NO_ERROR, "No error"},
    {NETMD_NOT_IMPLEMENTED, "Not implemented"},

    {NETMD_USB_OPEN_ERROR, "Error while opening the USB device"},
    {NETMD_USB_ERROR, "Generic USB error"},

    {NETMD_RESPONSE_TO_SHORT, "Response from device is shorter than expected."},
    {NETMD_RESPONSE_NOT_EXPECTED, "Response from device does not match with the expected result."}
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
