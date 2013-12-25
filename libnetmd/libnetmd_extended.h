/*
 * include this header file to get access to additional libnetmd members
 */

#include "libnetmd.h"

typedef struct {
        unsigned char content[255];
        size_t length;
        size_t position;
} netmd_response;

/*
 * additional members from secure.c
 */

void netmd_send_secure_msg(netmd_dev_handle *dev, unsigned char cmd, unsigned char *data, size_t data_size);
netmd_error netmd_recv_secure_msg(netmd_dev_handle *dev, unsigned char cmd, netmd_response *response,
                                  unsigned char expected_response_code);
netmd_error netmd_secure_real_recv_track(netmd_dev_handle *dev, uint32_t length, FILE *file, size_t chunksize);
void netmd_write_aea_header(char *name, uint32_t frames, unsigned char channel, FILE* f);
void netmd_write_wav_header(unsigned char format, uint32_t bytes, FILE *f);

/*
 * additional members from utils.c
 * XXX: do not include utils.h when using taglib, definition of min(a,b) is incomatible with definition of min(...) in taglib
 */
void netmd_check_response_bulk(netmd_response *response, const unsigned char* const expected,
                               const size_t expected_length, netmd_error *error);
void netmd_check_response_word(netmd_response *response, const uint16_t expected,
                               netmd_error *error);
void netmd_read_response_bulk(netmd_response *response, unsigned char* target,
                              const size_t length, netmd_error *error);
unsigned char *netmd_copy_word_to_buffer(unsigned char **buf, uint16_t value, int little_endian);
unsigned char netmd_read(netmd_response *response);
uint16_t netmd_read_word(netmd_response *response);
uint32_t netmd_read_doubleword(netmd_response *response);
