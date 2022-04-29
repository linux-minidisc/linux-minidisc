/*
 * Query formatting/scanning library for libnetmd -- simple unit tests
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

#include "query.h"
#include <stdio.h>
#include <string.h>

static void
dump_buffer(const struct netmd_bytebuffer *buffer)
{
    printf("buffer: %zu bytes\n", buffer->size);
    for (size_t i=0; i<buffer->size; ++i) {
        printf(" %02x", (int)(buffer->data[i] & 0xFF));
        if (i%32 == 31) {
            printf("\n");
        }
    }
    printf("\n");

}

#define EXPECT_BUFFER_EQUAL(query, result) \
    dump_buffer(query); \
    if (sizeof(result) != (query)->size) { \
        printf("Buffer size unexpected, expected %zu, got %zu\n", sizeof(result), (query)->size); \
        exit(1); \
    } else { \
        printf("Buffer size OK\n"); \
    } \
    if (memcmp(result, query->data, query->size) != 0) { \
        printf("Buffer data unexpected\n"); \
        exit(1); \
    } else { \
        printf("Buffer data OK\n"); \
    }

#define EXPECT_SCAN_QUERY_OK(scan_query) \
    if (scan_query) { \
        printf("%s: OK\n", #scan_query); \
    } else { \
        printf("%s: FAILED\n", #scan_query); \
        exit(1); \
    }

int main()
{
    // Test cases adapted from netmd-js
    // https://github.com/cybercase/netmd-js/blob/master/src/query-utils.test.ts

    // netmd_format_query

    {
        // format const
        struct netmd_bytebuffer *query = netmd_format_query("00 00 00 01");
        const char result[] = {0x00, 0x00, 0x00, 0x01};
        EXPECT_BUFFER_EQUAL(query, result);
        netmd_bytebuffer_free(query);
    }

    {
        // format string
        struct netmd_bytebuffer *str = netmd_bytebuffer_new_from_string("ciao");
        struct netmd_bytebuffer *query = netmd_format_query("00 00 00 %x", str);
        netmd_bytebuffer_free(str);
        const char result[] = {
            0x00,
            0x00,
            0x00, // header
            0x00,
            0x04, // length
            0x63,
            0x69,
            0x61,
            0x6f, // data
        };
        EXPECT_BUFFER_EQUAL(query, result);
        netmd_bytebuffer_free(query);
    }

    {
        // format byte array
        struct netmd_bytebuffer *str = netmd_bytebuffer_new();
        str = netmd_bytebuffer_append(str, ((const char[]){0xff, 0x00}), 2);
        struct netmd_bytebuffer *query = netmd_format_query("00 00 00 %*", str);
        netmd_bytebuffer_free(str);
        const char result[] = {
            0x00,
            0x00,
            0x00, // header
            0xff,
            0x00, // data
        };
        EXPECT_BUFFER_EQUAL(query, result);
        netmd_bytebuffer_free(query);
    }

    {
        // format bytearray from raw string
        struct netmd_bytebuffer *str = netmd_bytebuffer_new_from_string("abc");
        struct netmd_bytebuffer *query = netmd_format_query("00 00 00 %*", str);
        netmd_bytebuffer_free(str);
        const char result[] = {
            0x00,
            0x00,
            0x00, // header
            0x61,
            0x62,
            0x63, // string data
        };
        EXPECT_BUFFER_EQUAL(query, result);
        netmd_bytebuffer_free(query);
    }

    {
        // format null terminated string
        struct netmd_bytebuffer *query = netmd_format_query("00 00 00 %s", "ciao");
        const char result[] = {
            0x00,
            0x00,
            0x00, // header
            0x00,
            0x05, // length (2 bytes)
            0x63,
            0x69,
            0x61,
            0x6f,
            0x00, // string data
        };
        EXPECT_BUFFER_EQUAL(query, result);
        netmd_bytebuffer_free(query);
    }

    {
        // format byte
        struct netmd_bytebuffer *query = netmd_format_query("00 00 00 %b", 0xff);
        const char result[] = {0x00, 0x00, 0x00, 0xff};
        EXPECT_BUFFER_EQUAL(query, result);
        netmd_bytebuffer_free(query);
    }

    {
        // format byte
        struct netmd_bytebuffer *query = netmd_format_query("00 00 00 %b", 0xff);
        const char result[] = {0x00, 0x00, 0x00, 0xff};
        EXPECT_BUFFER_EQUAL(query, result);
        netmd_bytebuffer_free(query);
    }

    {
        // format word
        struct netmd_bytebuffer *query = netmd_format_query("00 00 00 %w", 0xff01);
        const char result[] = {0x00, 0x00, 0x00, 0xff, 0x01};
        EXPECT_BUFFER_EQUAL(query, result);
        netmd_bytebuffer_free(query);
    }

    {
        // format double word
        struct netmd_bytebuffer *query = netmd_format_query("00 00 00 %d", 0xff019922);
        const char result[] = {0x00, 0x00, 0x00, 0xff, 0x01, 0x99, 0x22};
        EXPECT_BUFFER_EQUAL(query, result);
        netmd_bytebuffer_free(query);
    }

    {
        // format quad word
        struct netmd_bytebuffer *query = netmd_format_query("00 00 00 %q", 0xff019922229901ffull);
        const char result[] = {0x00, 0x00, 0x00, 0xff, 0x01, 0x99, 0x22, 0x22, 0x99, 0x01, 0xff};
        EXPECT_BUFFER_EQUAL(query, result);
        netmd_bytebuffer_free(query);
    }

    // netmd_scan_query

    {
        // parse const
        const char query[] = {0x00, 0x00, 0x00, 0x01};
        EXPECT_SCAN_QUERY_OK(netmd_scan_query(query, sizeof(query), "00 00 00 01"));
    }

    {
        // parse wildcard
        const char query[] = {0x00, 0x00, 0x00, 0x01};
        EXPECT_SCAN_QUERY_OK(netmd_scan_query(query, sizeof(query), "00 00 %? 01"));
    }

    {
        // parse remainings
        const char query[] = {0x00, 0x00, 0x00, 0x01};
        struct netmd_bytebuffer *out = netmd_bytebuffer_new();
        EXPECT_SCAN_QUERY_OK(netmd_scan_query(query, sizeof(query), "00 00 %*", &out));
        if (out->size != 2 || out->data[0] != 0x00 || out->data[1] != 0x01) {
            printf("Unexpected data\n");
            exit(1);
        }
        dump_buffer(out);
        netmd_bytebuffer_free(out);
    }

    {
        // parse string
        const char query[] = {
            0x00,
            0x00, // header
            0x00,
            0x04, // length
            0x63,
            0x69,
            0x61,
            0x6f, // data
        };

        struct netmd_bytebuffer *out = netmd_bytebuffer_new();
        EXPECT_SCAN_QUERY_OK(netmd_scan_query(query, sizeof(query), "00 00 %x", &out));

        const char *str = netmd_bytebuffer_as_string(&out);
        printf("Got string: \"%s\"\n", str);

        if (out->size != 4 || memcmp(out->data, "ciao", 4) != 0) {
            printf("Unexpected data\n");
            exit(1);
        }
        dump_buffer(out);
        netmd_bytebuffer_free(out);
    }

    {
        // parse null-terminated string
        const char query[] = {
            0x00,
            0x00, // header
            0x00,
            0x05, // length
            0x63,
            0x69,
            0x61,
            0x6f, // data
            0x00, // terminator
        };

        const char *str = NULL;
        EXPECT_SCAN_QUERY_OK(netmd_scan_query(query, sizeof(query), "00 00 %s", &str));

        printf("Got string: \"%s\"\n", str);

        if (strcmp(str, "ciao") != 0) {
            printf("Unexpected data\n");
            exit(1);
        }
    }

    {
        // parse byte
        const char query[] = {0x00, 0x00, 0xff};

        uint8_t byte = 0;
        EXPECT_SCAN_QUERY_OK(netmd_scan_query(query, sizeof(query), "00 00 %b", &byte));

        if (byte != 0xff) {
            printf("Unexpected data\n");
            exit(1);
        }
    }

    {
        // parse word
        const char query[] = {0x00, 0x00, 0xff, 0x01};

        uint16_t word = 0;
        EXPECT_SCAN_QUERY_OK(netmd_scan_query(query, sizeof(query), "00 00 %w", &word));

        if (word != 0xff01) {
            printf("Unexpected data\n");
            exit(1);
        }
    }

    {
        // parse double word
        const char query[] = {0x00, 0x00, 0xff, 0x01, 0xaa, 0x10};

        uint32_t doubleword = 0;
        EXPECT_SCAN_QUERY_OK(netmd_scan_query(query, sizeof(query), "00 00 %d", &doubleword));

        if (doubleword != 0xff01aa10) {
            printf("Unexpected data\n");
            exit(1);
        }
    }

    {
        // parse quadword
        const char query[] = {0x00, 0x00, 0xff, 0x01, 0xaa, 0x10, 0x10, 0xaa, 0x01, 0xff};

        uint64_t quadword = 0;
        EXPECT_SCAN_QUERY_OK(netmd_scan_query(query, sizeof(query), "00 00 %q", &quadword));

        if (quadword != 0xff01aa1010aa01ffull) {
            printf("Unexpected data\n");
            exit(1);
        }
    }

    printf("All tests successful.\n");

    return 0;
}
