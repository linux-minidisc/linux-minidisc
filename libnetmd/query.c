/*
 * Query formatting/scanning library for libnetmd
 * Based on netmd.py and netmd-js code
 * Copyright (c) 2022, Thomas Perl <m@thp.io>
 *
 * Portions based on netmd-js:
 * https://github.com/cybercase/netmd-js
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


#include "query.h"
#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>



struct netmd_bytebuffer *
netmd_bytebuffer_append(struct netmd_bytebuffer *buffer, const char *data, size_t size)
{
    if (buffer->size + size >= buffer->allocated) {
        while (buffer->size + size >= buffer->allocated) {
            buffer->allocated *= 2;
        }

        buffer = realloc(buffer, sizeof(buffer) + buffer->allocated);
    }

    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;

    return buffer;
}

static struct netmd_bytebuffer *
netmd_bytebuffer_append_byte(struct netmd_bytebuffer *buffer, uint8_t byte)
{
    return netmd_bytebuffer_append(buffer, (const char *)&byte, 1);
}

struct netmd_bytebuffer *
netmd_bytebuffer_new(void)
{
    size_t default_allocation = 128;

    struct netmd_bytebuffer *buffer = malloc(sizeof(buffer) + default_allocation);
    buffer->size = 0;
    buffer->allocated = default_allocation;

    return buffer;
}

struct netmd_bytebuffer *
netmd_bytebuffer_new_from_string(const char *str)
{
    struct netmd_bytebuffer *result = netmd_bytebuffer_new();

    size_t len = strlen(str) + 1;

    result = netmd_bytebuffer_append(result, str, len);

    // We copied the '\0'-terminator, so the data can be used as C string, but
    // we don't want the '\0'-terminator to be part of the byte buffer size.
    result->size--;

    return result;
}

struct netmd_bytebuffer *
netmd_format_query(const char *fmt, ...)
{
    struct netmd_bytebuffer *query = netmd_bytebuffer_new();

    va_list args;
    va_start(args, fmt);

    int half = -1;
    bool escaped = false;

    const char *cur = fmt;

    while (*cur) {
        char ch = *cur++;

        if (escaped) {
            escaped = false;

            if (ch == 'b') {
                uint8_t value = va_arg(args, int);
                query = netmd_bytebuffer_append_byte(query, value);
            } else if (ch == 'w') {
                uint16_t value = va_arg(args, int);
                query = netmd_bytebuffer_append_byte(query, (value >> 8) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, value & 0xFF);
            } else if (ch == 'd') {
                uint32_t value = va_arg(args, uint32_t);
                query = netmd_bytebuffer_append_byte(query, (value >> 24) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, (value >> 16) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, (value >> 8) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, value & 0xFF);
            } else if (ch == 'q') {
                uint64_t value = va_arg(args, uint64_t);
                query = netmd_bytebuffer_append_byte(query, (value >> 56) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, (value >> 48) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, (value >> 40) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, (value >> 32) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, (value >> 24) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, (value >> 16) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, (value >> 8) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, value & 0xFF);
            } else if (ch == 'x') {
                // Append byte array with 16-bit length prefixed
                struct netmd_bytebuffer *buf = va_arg(args, struct netmd_bytebuffer *);

                uint16_t len = buf->size;
                query = netmd_bytebuffer_append_byte(query, (len >> 8) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, len & 0xFF);

                for (uint16_t i=0; i<len; ++i) {
                    query = netmd_bytebuffer_append_byte(query, buf->data[i]);
                }
            } else if (ch == 's') {
                // Append zero-terminated string with 16-bit length prefixed
                const char *str = va_arg(args, const char *);

                uint16_t len = strlen(str) + 1;
                query = netmd_bytebuffer_append_byte(query, (len >> 8) & 0xFF);
                query = netmd_bytebuffer_append_byte(query, len & 0xFF);

                query = netmd_bytebuffer_append(query, str, len);
            } else if (ch == '*') {
                // Append byte buffer directly
                struct netmd_bytebuffer *buf = va_arg(args, struct netmd_bytebuffer *);

                query = netmd_bytebuffer_append(query, buf->data, buf->size);
            } else {
                netmd_log(NETMD_LOG_ERROR, "Invalid query format: '%s', ch == '%c'\n", fmt, ch);
                exit(1);
            }

            continue;
        }

        if (ch == '%') {
            if (half != -1) {
                netmd_log(NETMD_LOG_ERROR, "Invalid query format: '%s', half='%c'\n", fmt, half);
                exit(1);
            }

            escaped = true;
            continue;
        }

        if (ch == ' ') {
            continue;
        }

        if (half == -1) {
            half = ch;
        } else {
            char tmp[3] = { half, ch, '\0' };
            int value;
            sscanf(tmp, "%x", &value);
            netmd_bytebuffer_append_byte(query, (uint8_t)value);
            half = -1;
        }
    }

    va_end(args);

    return query;
}

bool
netmd_scan_query(const char *data, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int half = -1;
    bool escaped = false;

    const uint8_t *read_ptr = (const uint8_t *)data;
    const uint8_t *end_ptr = read_ptr + size;

    const char *cur = fmt;

    while (*cur && read_ptr < end_ptr) {
        char ch = *cur++;

        if (escaped) {
            escaped = false;

            if (ch == '?') {
                // wildcard byte
                read_ptr++;
            } else if (ch == 'b') {
                uint8_t *value = va_arg(args, uint8_t *);

                *value = *read_ptr++;
            } else if (ch == 'w') {
                uint16_t *value = va_arg(args, uint16_t *);

                if (read_ptr + 2 > end_ptr) {
                    netmd_log(NETMD_LOG_ERROR, "Data underrun while reading word\n");
                    return false;
                }

                *value = (read_ptr[0] << 8) | read_ptr[1];

                read_ptr += 2;
            } else if (ch == 'd') {
                uint32_t *value = va_arg(args, uint32_t *);

                if (read_ptr + 4 > end_ptr) {
                    netmd_log(NETMD_LOG_ERROR, "Data underrun while reading doubleword\n");
                    return false;
                }

                *value = (read_ptr[0] << 24) | (read_ptr[1] << 16) | (read_ptr[2] << 8) | read_ptr[3];

                read_ptr += 4;
            } else if (ch == 'q') {
                uint64_t *value = va_arg(args, uint64_t *);

                if (read_ptr + 8 > end_ptr) {
                    netmd_log(NETMD_LOG_ERROR, "Data underrun while reading quadword\n");
                    return false;
                }

                *value = 0;
                for (int i=0; i<8; ++i) {
                    *value <<= 8;
                    *value |= *read_ptr++;
                }
            } else if (ch == 'x') {
                struct netmd_bytebuffer **buf = va_arg(args, struct netmd_bytebuffer **);

                if (read_ptr + 2 > end_ptr) {
                    netmd_log(NETMD_LOG_ERROR, "Data underrun while reading byte buffer length\n");
                    return false;
                }

                uint16_t len = (read_ptr[0] << 8) | read_ptr[1];
                read_ptr += 2;

                if (read_ptr + len > end_ptr) {
                    netmd_log(NETMD_LOG_ERROR, "Data underrun while reading byte buffer payload\n");
                    return false;
                }

                *buf = netmd_bytebuffer_append(*buf, (const char *)read_ptr, len);
                read_ptr += len;
            } else if (ch == 's') {
                // zero-terminated string with 16-bit length prefixed
                const char **str = va_arg(args, const char **);

                if (read_ptr + 2 > end_ptr) {
                    netmd_log(NETMD_LOG_ERROR, "Data underrun while reading zero-terminated string length\n");
                    return false;
                }

                uint16_t len = (read_ptr[0] << 8) | read_ptr[1];
                read_ptr += 2;

                if (read_ptr + len > end_ptr) {
                    netmd_log(NETMD_LOG_ERROR, "Data underrun while reading zero-terminated string payload\n");
                    return false;
                }

                // We can just reference the buffer data directly,
                // as it's zero-terminated
                *str = (const char *)read_ptr;

                // Read over the string data
                read_ptr += len - 1;

                // Read the zero terminator explicitly
                if (*read_ptr++ != '\0') {
                    netmd_log(NETMD_LOG_ERROR, "Null terminator for string missing\n");
                    return false;
                }
            } else if (ch == '*') {
                // Append byte buffer directly
                struct netmd_bytebuffer **buf = va_arg(args, struct netmd_bytebuffer **);

                size_t remaining = end_ptr - read_ptr;

                *buf = netmd_bytebuffer_append(*buf, (const char *)read_ptr, remaining);
                read_ptr += remaining;
            } else {
                netmd_log(NETMD_LOG_ERROR, "Invalid query format: '%s', ch == '%c'\n", fmt, ch);
                return false;
            }

            continue;
        }

        if (ch == '%') {
            if (half != -1) {
                netmd_log(NETMD_LOG_ERROR, "Invalid query format: '%s', half='%c'\n", fmt, half);
                return false;
            }

            escaped = true;
            continue;
        }

        if (ch == ' ') {
            continue;
        }

        if (half == -1) {
            half = ch;
        } else {
            char tmp[3] = { half, ch, '\0' };
            int value;
            sscanf(tmp, "%x", &value);

            if (*read_ptr != value) {
                netmd_log(NETMD_LOG_ERROR, "Expected 0x%02x, got 0x%02x\n", value, *read_ptr);
                return false;
            }

            ++read_ptr;
            half = -1;
        }
    }

    va_end(args);

    return (read_ptr == end_ptr && *cur == '\0');
}

const char *
netmd_bytebuffer_as_string(struct netmd_bytebuffer **buffer)
{
    // Check if the (already-allocated) byte beyond the "used" portion of the buffer is '\0'
    if ((*buffer)->allocated > (*buffer)->size && (*buffer)->data[(*buffer)->size] == '\0') {
        return (*buffer)->data;
    }

    // Append the '\0'-character right after the end of the string
    *buffer = netmd_bytebuffer_append_byte(*buffer, '\0');

    // Revert the buffer size change from above
    (*buffer)->size--;

    return (*buffer)->data;
}

void
netmd_bytebuffer_free(struct netmd_bytebuffer *buffer)
{
    free(buffer);
}
