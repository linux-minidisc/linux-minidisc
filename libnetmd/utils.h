#ifndef UTILS_H
#define UTILS_H

/** \file utils.h */

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "error.h"

typedef struct {
        unsigned char content[255];
        size_t length;
        size_t position;
} netmd_response;

#ifndef min
    #define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifdef WIN32
    #include <windows.h>
    #define msleep(x) Sleep(x)
#else
    #define msleep(x) usleep(1000*x)
#endif

unsigned char proper_to_bcd_single(unsigned char value);
unsigned char* proper_to_bcd(unsigned int value, unsigned char* target, size_t len);
unsigned char bcd_to_proper_single(unsigned char value);
unsigned int bcd_to_proper(unsigned char* value, size_t len);

void netmd_check_response_bulk(netmd_response *response, const unsigned char* const expected,
                               const size_t expected_length, netmd_error *error);

void netmd_check_response_word(netmd_response *response, const uint16_t expected,
                               netmd_error *error);

void netmd_check_response(netmd_response *response, const unsigned char expected,
                          netmd_error *error);

void netmd_read_response_bulk(netmd_response *response, unsigned char* target,
                              const size_t length, netmd_error *error);



unsigned char *netmd_copy_word_to_buffer(unsigned char **buf, uint16_t value, int little_endian);
unsigned char *netmd_copy_doubleword_to_buffer(unsigned char **buf, uint32_t value, int little_endian);
unsigned char *netmd_copy_quadword_to_buffer(unsigned char **buf, uint64_t value);

unsigned char netmd_read(netmd_response *response);
uint16_t netmd_read_word(netmd_response *response);
uint32_t netmd_read_doubleword(netmd_response *response);
uint64_t netmd_read_quadword(netmd_response *response);

uint16_t netmd_convert_word(uint16_t value);

#endif
