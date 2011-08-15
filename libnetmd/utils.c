#include <string.h>

#include "utils.h"
#include "log.h"

inline int min(int a,int b)
{
    if (a < b) {
        return a;
    }

    return b;
}

inline unsigned char proper_to_bcd_single(unsigned char value)
{
    unsigned char high, low;

    low = (value % 10) & 0xf;
    high = (((value / 10) % 10) * 0xfU) & 0xf0;

    return high | low;
}

inline unsigned char* proper_to_bcd(unsigned int value, unsigned char* target, size_t len)
{
    while (value > 0 && len > 0) {
        target[len - 1] = proper_to_bcd_single(value & 0xff);
        value /= 100;
        len--;
    }

    return target;
}

inline unsigned char bcd_to_proper_single(unsigned char value)
{
    unsigned char high, low;

    high = (value & 0xf0) >> 4;
    low = (value & 0xf);

    return ((high * 10U) | low) & 0xff;
}

inline unsigned int bcd_to_proper(unsigned char* value, size_t len)
{
    unsigned int result = 0;
    unsigned int nibble_value = 1;

    for (; len > 0; len--) {
        result += nibble_value * bcd_to_proper_single(value[len - 1]);

        nibble_value *= 100;
    }

    return result;
}

void netmd_check_response_bulk(netmd_response *response, const unsigned char* const expected,
                               const size_t expected_length, netmd_error *error)
{
    unsigned char *current;

    /* only check if there was no error before */
    if (*error == NETMD_NO_ERROR) {

        if ((response->length - response->position) < expected_length) {
            *error = NETMD_RESPONSE_TO_SHORT;
        }
        else {
            current = response->content + response->position;

            if (memcmp(current, expected, expected_length) == 0) {
                response->position += expected_length;
            }
            else {
                netmd_log_hex(0, current, expected_length);
                netmd_log_hex(0, expected, expected_length);
                *error = NETMD_RESPONSE_NOT_EXPECTED;
            }
        }
    }
}

void netmd_check_response_word(netmd_response *response, const uint16_t expected,
                               netmd_error *error)
{
    unsigned char buf[2];
    unsigned char *tmp = buf;

    /* only check if there was no error before */
    if (*error == NETMD_NO_ERROR) {
        if ((response->length - response->position) < 2) {
            *error = NETMD_RESPONSE_TO_SHORT;
        }
        else {
            netmd_copy_word_to_buffer(&tmp, expected);
            netmd_check_response_bulk(response, buf, 2, error);
        }
    }
}

void netmd_check_response_doubleword(netmd_response *response, const uint32_t expected,
                                     netmd_error *error)
{
    unsigned char buf[4];
    unsigned char *tmp = buf;

    /* only check if there was no error before */
    if (*error == NETMD_NO_ERROR) {
        if ((response->length - response->position) < 4) {
            *error = NETMD_RESPONSE_TO_SHORT;
        }
        else {
            netmd_copy_doubleword_to_buffer(&tmp, expected);
            netmd_check_response_bulk(response, buf, 4, error);
        }
    }
}


void netmd_check_response(netmd_response *response, const unsigned char expected,
                          netmd_error *error)
{
    /* only check if there was no error before */
    if (*error == NETMD_NO_ERROR) {

        if ((response->length - response->position) < 1) {
            *error = NETMD_RESPONSE_TO_SHORT;
        }
        else {
            if (response->content[response->position] == expected) {
                response->position++;
            }
            else {
                netmd_log_hex(0, response->content + response->position, 1);
                netmd_log_hex(0, &expected, 1);
                *error = NETMD_RESPONSE_NOT_EXPECTED;
            }
        }
    }
}

void netmd_read_response_bulk(netmd_response *response, unsigned char* target,
                              const size_t length, netmd_error *error)
{
    /* only copy if there was no error before */
    if (*error == NETMD_NO_ERROR) {

        if ((response->length - response->position) < length) {
            *error = NETMD_RESPONSE_TO_SHORT;
        }
        else {
            if (target) {
                memcpy(target, response->content + response->position, length);
            }

            response->position += length;
        }
    }
}

unsigned char *netmd_copy_word_to_buffer(unsigned char **buf, uint16_t value)
{
    **buf = (unsigned char)((value >> 8) & 0xff);
    (*buf)++;

    **buf = (unsigned char)((value >> 0) & 0xff);
    (*buf)++;

    return *buf;
}

unsigned char *netmd_copy_doubleword_to_buffer(unsigned char **buf, uint32_t value)
{
    **buf = (unsigned char)(value >> 24) & 0xff;
    (*buf)++;

    **buf = (value >> 16) & 0xff;
    (*buf)++;

    **buf = (value >> 8) & 0xff;
    (*buf)++;

    **buf = (value >> 0) & 0xff;
    (*buf)++;

    return *buf;
}

unsigned char *netmd_copy_quadword_to_buffer(unsigned char **buf, uint64_t value)
{
    **buf = (value >> 56) & 0xff;
    (*buf)++;

    **buf = (value >> 48) & 0xff;
    (*buf)++;

    **buf = (value >> 40) & 0xff;
    (*buf)++;

    **buf = (value >> 32) & 0xff;
    (*buf)++;

    **buf = (value >> 24) & 0xff;
    (*buf)++;

    **buf = (value >> 16) & 0xff;
    (*buf)++;

    **buf = (value >> 8) & 0xff;
    (*buf)++;

    **buf = (value >> 0) & 0xff;
    (*buf)++;

    return *buf;
}

uint16_t netmd_read_word(netmd_response *response)
{
    int i;
    uint16_t value;

    value = 0;
    for (i = 0; i < 2; i++) {
        value = (((unsigned int)value << 8U) | ((unsigned int)response->content[response->position] & 0xff)) & 0xffff;
        response->position++;
    }

    return value;
}

uint32_t netmd_read_doubleword(netmd_response *response)
{
    int i;
    uint32_t value;

    value = 0;
    for (i = 0; i < 4; i++) {
        value <<= 8;
        value += (response->content[response->position] & 0xff);
        response->position++;
    }

    return value;
}

uint64_t netmd_read_quadword(netmd_response *response)
{
    int i;
    uint64_t value;

    value = 0;
    for (i = 0; i < 8; i++) {
        value <<= 8;
        value += (response->content[response->position] & 0xff);
        response->position++;
    }

    return value;
}
