#pragma once

/** \file query.h */

/*
 * Query formatting/scanning library for libnetmd
 * Based on netmd.py and netmd-js code
 * Copyright (c) 2022, Thomas Perl <m@thp.io>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Flexible byte buffer with length.
 */
struct netmd_bytebuffer {
    size_t size; /*!< Current logical size of the byte buffer */
    size_t allocated; /*!< Number of data bytes allocated */
    char data[]; /*!< Data bytes, <allocated> bytes available */
};

/**
 * Format a query.
 *
 * The format string is a string of hex characters, each pair representing
 * a single byte. Spaces can be used to visually group things, but are ignored.
 *
 * The following format characters are supported:
 *
 *   - \%b ... Write a "uint8_t" argument
 *   - \%w ... Write a "uint16_t" argument (in big-endian byte order)
 *   - \%d ... Write a "uint32_t" argument (in big-endian byte order)
 *   - \%q ... Write a "uint64_t" argument (in big-endian byte order)
 *   - \%x ... Write the bytes contained in the "struct netmd_bytebuffer *" argument (with 16-bit length prefixed in big-endian)
 *   - \%s ... Write a C string ("const char *" argument) as bytes (with 16-bit length prefixed in big-endian, including '\0'-terminator)
 *   - \%* ... Write the bytes contained in the "struct netmd_bytebuffer *" argument directly (no length prefix, no '\0'-terminator)
 *
 * @param fmt Format string
 * @param ... Values for the format string to consume
 * @return A newly-allocated byte buffer that should be free'd with netmd_bytebuffer_free()
 */
struct netmd_bytebuffer *
netmd_format_query(const char *fmt, ...);


/**
 * Scan bytes using a query.
 *
 * The format string is a string of hex characters, each pair representing
 * a single byte. Spaces can be used to visually group things, but are ignored.
 *
 * The following format characters are supported:
 *
 *   - \%? ... Any single-byte value, the byte is consumed, but ignored
 *   - \%b ... Read a single byte and place it in the "uint8_t *" out parameter
 *   - \%w ... Read a 2-byte value in big-endian, place it in the "uint16_t *" out parameter
 *   - \%d ... Read a 4-byte value in big-endian, place it in the "uint32_t *" out parameter
 *   - \%q ... Read a 8-byte value in big-endian, place it in the "uint64_t *" out parameter
 *   - \%x ... Read bytes with 16-bit length prefix, place it in the "struct netmd_bytebuffer **" out parameter
 *   - \%s ... Read a C string with 16-bit length prefix, place it in the "const char **" out parameter (will point into the input buffer)
 *   - \%* ... Put all remaining bytes into a "struct netmd_bytebuffer **" out parameter (can appear at most once, at the end of the format string)
 *
 * @param data The data to scan
 * @param size Number of bytes in data
 * @param fmt Format string
 * @param ... Out parameters referenced by the format string
 * @return true if the scan was successful, false if there was an error
 */
bool
netmd_scan_query(const char *data, size_t size, const char *fmt, ...);


/**
 * Create a new, empty byte buffer.
 *
 * @return A newly-allocated byte buffer with zero size. Free with netmd_bytebuffer_free().
 */
struct netmd_bytebuffer *
netmd_bytebuffer_new(void);


/**
 * Append data to a byte buffer, growing it as necessary.
 *
 * @param buffer The buffer to append the bytes to
 * @param data Pointer to the data to append to
 * @param size Number of bytes to append
 * @return The byte buffer, which might now have a different address if it has been re-allocated.
 */
struct netmd_bytebuffer *
netmd_bytebuffer_append(struct netmd_bytebuffer *buffer, const char *data, size_t size);


/**
 * Create a new byte buffer holding a zero-terminated string.
 *
 * The length will be the string length without the zero-terminator.
 * For convenience, the byte buffer's allocated data will still be
 * terminated with a '\0' byte, so the data can be used as a C string.
 *
 * @param str The C string
 * @return A newly-allocated byte buffer containing the string, free with netmd_bytebuffer_free()
 */
struct netmd_bytebuffer *
netmd_bytebuffer_new_from_string(const char *str);


/**
 * Return a string version of the byte buffer's contents.
 *
 * The returned string is owned by the byte buffer, and is only
 * valid as long as the byte buffer isn't modified and not free'd.
 *
 * Because retrieving the contents as string might need to add
 * a zero terminator, and this might cause a re-allocation, a
 * pointer to the byte buffer pointer must be passed in.
 *
 * @param buffer A pointer to the byte buffer pointer
 * @return A zero-terminated string, pointing into the byte buffer
 */
const char *
netmd_bytebuffer_as_string(struct netmd_bytebuffer **buffer);


/**
 * Free a previously-allocated byte buffer.
 *
 * @param buffer Buffer to be free'd
 */
void
netmd_bytebuffer_free(struct netmd_bytebuffer *buffer);

#ifdef __cplusplus
}
#endif
