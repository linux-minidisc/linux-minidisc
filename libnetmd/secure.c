/*
  Copyright 2004 Bertrik Sikken (bertrik@zonnet.nl)

  Set of NetMD commands that start with the sequence
  0x00,0x18,0x00,0x08,0x00,0x46,0xf0,0x03,0x01,0x03

  These commands are used during check-in/check-out.
*/

#include <string.h>
#include <stdio.h>
#include <usb.h>
#include <rpc/des_crypt.h>

#include "secure.h"
#include "const.h"
#include "utils.h"
#include "log.h"

static const unsigned char secure_header[] = { 0x18, 0x00, 0x08, 0x00, 0x46,
                                               0xf0, 0x03, 0x01, 0x03 };

void build_request(unsigned char *request, const unsigned char cmd, unsigned char *data, const size_t data_size)
{
    size_t header_length;
    header_length = 1 + sizeof(secure_header);


    request[0] = NETMD_STATUS_CONTROL;

    memcpy(request + 1, secure_header, sizeof(secure_header));

    request[header_length] = cmd;
    request[header_length + 1] = 0xff;

    memcpy(request + header_length + 2, data, data_size);
}

void execute_request(netmd_dev_handle *dev, const unsigned char cmd, unsigned char *data,
                     const size_t data_size, netmd_response *response)
{
    unsigned char *request;
    size_t request_length;

    /* alloc memory */
    request_length = 1 + sizeof(secure_header) + 2 + data_size;
    request = malloc(request_length);

    /* build request */
    build_request(request, cmd, data, data_size);

    /* execute */
    response->length = (size_t)netmd_exch_message(dev, request, request_length,
                                                  response->content);
    response->position = 0;

    /* free memory */
    free(request);
    request = NULL;
}


netmd_error parse_netmd_return_status(unsigned char status)
{
    switch (status) {
    case NETMD_STATUS_NOT_IMPLEMENTED:
        return NETMD_COMMAND_FAILED_NOT_IMPLEMENTED;

    case NETMD_STATUS_REJECTED:
        return NETMD_COMMAND_FAILED_REJECTED;

    case NETMD_STATUS_ACCEPTED:
        break;

    case NETMD_STATUS_IN_TRANSITION:
        break;

    case NETMD_STATUS_IMPLEMENTED:
        break;

    case NETMD_STATUS_CHANGED:
        break;

    case NETMD_STATUS_INTERIM:
        break;

    default:
        return NETMD_COMMAND_FAILED_UNKNOWN_ERROR;
    }

    return NETMD_NO_ERROR;
}

/** Helper function to make life a little simpler for other netmd_secure_cmd_* functions */
netmd_error exch_secure_msg(netmd_dev_handle *dev, unsigned char cmd, unsigned char *data, size_t data_size, netmd_response *response)
{
    netmd_error error;

    /* execure request */
    execute_request(dev, cmd, data, data_size, response);

    if (response->length < 1) {
        return NETMD_COMMAND_FAILED_NO_RESPONSE;
    }

    error = parse_netmd_return_status(response->content[0]);
    response->position = 1;

    netmd_check_response_bulk(response, secure_header, sizeof(secure_header), &error);
    netmd_check_response(response, cmd, &error);
    netmd_check_response(response, 0x00, &error);

    return error;
}

/*
  => 00  18 00 08 00 46 f0 03 01 03  80 ff
  <= 09  18 00 08 00 46 f0 03 01 03  80 00
*/
netmd_error netmd_secure_enter_session(netmd_dev_handle *dev)
{
    netmd_response response;
    return exch_secure_msg(dev, 0x80, NULL, 0, &response);
}

/*
  => 00  18 00 08 00 46 f0 03 01 03  81 ff
  <= 09  18 00 08 00 46 f0 03 01 03  81 00
*/
netmd_error netmd_secure_leave_session(netmd_dev_handle *dev)
{
    netmd_response response;
    return exch_secure_msg(dev, 0x81, NULL, 0, &response);
}

/*
  => 00  18 00 08 00 46 f0 03 01 03  11 ff
  <= 09  18 00 08 00 46 f0 03 01 03  11 00  01 00 00 21 cf 06 00 00
*/
netmd_error netmd_secure_get_leaf_id(netmd_dev_handle *dev, uint64_t *player_id)
{
    netmd_response response;
    netmd_error error;

    error = exch_secure_msg(dev, 0x11, NULL, 0, &response);
    if (error == NETMD_NO_ERROR) {
        *player_id = netmd_read_quadword(&response);
    }

    return error;
}

uint16_t netmd_get_chain_length(netmd_keychain *chain)
{
    uint16_t length;

    length = 0;
    while (chain != NULL) {
        chain = chain->next;
        length++;
    }

    return length;
}

void netmd_build_send_key_data_command(unsigned char *buf, uint16_t databytes,
                                       uint16_t chain_length,
                                       uint32_t key_depth, uint32_t key_id,
                                       netmd_keychain *chain, char *signature)
{
    netmd_copy_word_to_buffer(&buf, databytes);
    netmd_copy_word_to_buffer(&buf, 0);
    netmd_copy_word_to_buffer(&buf, databytes);
    netmd_copy_word_to_buffer(&buf, 0);

    /* data */
    netmd_copy_word_to_buffer(&buf, chain_length);
    netmd_copy_doubleword_to_buffer(&buf, key_depth);
    netmd_copy_doubleword_to_buffer(&buf, key_id);
    netmd_copy_doubleword_to_buffer(&buf, 0);

    /* add all keys from the keychain to the buffer */
    while (chain != NULL) {
        memcpy(&buf, chain->key, 16);
        buf += 16;

        chain = chain->next;
    }

    memcpy(&buf, signature, 24);
}

/*
  => 00  18 00 08 00 46 f0 03 01 03  12 ff  00 38  00 00  00 38  00 00  00 01
     00 00 00 09  00 01 00 01  00 00 00 00  01 ca be 07 2c 4d a7 ae f3 6c 8d 73
     fa 60 2b d1 0f f4 7d 45 9c 72 da 81 85 16 9d 73 49 00 ff 6c 6a b9 61 6b 03
     04 f9 ce
  <= 09  18 00 08 00 46 f0 03 01 03  12 00  00 38  00 00  00 38  00 00
*/
netmd_error netmd_secure_send_key_data(netmd_dev_handle *dev, netmd_ekb* ekb)
{
    unsigned char *cmd;
    size_t size;
    uint16_t chain_length;
    netmd_response response;
    netmd_error error;

    chain_length = netmd_get_chain_length(ekb->chain);
    size = 22 + ((unsigned int)chain_length*16) + 24;

    cmd = malloc(size);
    netmd_build_send_key_data_command(cmd, (size - 8) & 0xffff, chain_length, ekb->depth,
                                      ekb->id, ekb->chain, ekb->signature);

    error = exch_secure_msg(dev, 0x12, cmd, size, &response);
    free(cmd);

    netmd_check_response_word(&response, (size - 8) & 0xffff, &error);
    netmd_check_response_word(&response, 0, &error);
    netmd_check_response_word(&response, (size - 8) & 0xffff, &error);
    netmd_check_response_word(&response, 0, &error);

    return error;
}

netmd_error netmd_secure_session_key_exchange(netmd_dev_handle *dev,
                                              unsigned char *rand_in,
                                              unsigned char *rand_out)
{
    unsigned char cmdhdr[] = {0x00, 0x00, 0x00};
    unsigned char cmd[sizeof(cmdhdr) + 8];

    netmd_response response;
    netmd_error error;

    memcpy(cmd, cmdhdr, sizeof(cmdhdr));
    memcpy(cmd + sizeof(cmdhdr), rand_in, 8);

    error = exch_secure_msg(dev, 0x20, cmd, sizeof(cmd), &response);

    netmd_check_response(&response, 0x00, &error);
    netmd_check_response(&response, 0x00, &error);
    netmd_check_response(&response, 0x00, &error);
    netmd_read_response_bulk(&response, rand_out, 8, &error);

    return error;
}


netmd_error netmd_secure_session_key_forget(netmd_dev_handle *dev)
{
    unsigned char cmd[] = {0x00, 0x00, 0x00};
    netmd_response response;
    netmd_error error;

    error = exch_secure_msg(dev, 0x21, cmd, sizeof(cmd), &response);
    netmd_check_response_bulk(&response, cmd, sizeof(cmd), &error);
    return error;
}

netmd_error netmd_secure_setup_download(netmd_dev_handle *dev,
                                        unsigned char *contentid,
                                        unsigned char *key_encryption_key,
                                        unsigned char *sessionkey)
{
    unsigned char cmdhdr[] = { 0x00, 0x00 };
    unsigned char data[32] = { 0x01, 0x01, 0x01, 0x01 /* ... */};
    unsigned char cmd[sizeof(cmdhdr) + sizeof(data)];
    unsigned char iv[8] = { 0 };

    int des_error;
    netmd_response response;
    netmd_error error;

    memcpy(data + 4, contentid, 20);
    memcpy(data + 24, key_encryption_key, 8);
    des_error = cbc_crypt((char*)sessionkey, (char*)data, sizeof(data),
                          DES_ENCRYPT, (char*)iv);
    if (DES_FAILED(des_error)) {
        return NETMD_DES_ERROR;
    }

    memcpy(cmd, cmdhdr, sizeof(cmdhdr));
    memcpy(cmd + sizeof(cmdhdr), data, 32);

    error = exch_secure_msg(dev, 0x22, cmd, sizeof(cmd), &response);
    netmd_check_response_bulk(&response, cmdhdr, sizeof(cmdhdr), &error);
    return error;
}


/** Prepare USB download?
    \param dev USB device handle
    \param track_type SP=0x006, LP2=0x9402, LP4=0xA800
    \param length_byte ??? 0x58 and 0x5A have been seen here
    \param length length of USB download
    \param *track_nr new track number
*/
int netmd_secure_cmd_28(netmd_dev_handle *dev, unsigned int track_type,
                        unsigned int length_byte, size_t length,
                        unsigned int *track_nr)
{
    unsigned char cmdhdr[] = {0x00, 0x01, 0x00, 0x10,
                              0x01, 0xff, 0xff, 0x00};
    unsigned char cmd[30];
    size_t cmd_length;
    int ret;
    netmd_response response;

    cmd_length = 0;
    memcpy(cmd, cmdhdr, sizeof(cmdhdr));
    cmd_length += sizeof(cmdhdr);
    cmd[cmd_length++] = (track_type >> 8) & 0xff;
    cmd[cmd_length++] = (track_type >> 0) & 0xff;
    cmd[cmd_length++] = 0;
    cmd[cmd_length++] = 0;
    cmd[cmd_length++] = (length_byte >> 8) & 0xff;
    cmd[cmd_length++] = (length_byte >> 0) & 0xff;
    cmd[cmd_length++] = (unsigned char)((length >> 24) & 0xff);
    cmd[cmd_length++] = (length >> 16) & 0xff;
    cmd[cmd_length++] = (length >> 8) & 0xff;
    cmd[cmd_length++] = (length >> 0) & 0xff;

    ret = exch_secure_msg(dev, 0x28, cmd, cmd_length, &response);
    if (ret > 0) {
            *track_nr = (unsigned int)(response.content[17] << 8) | response.content[18];
    }
    return ret;
}

netmd_error netmd_secure_commit_track(netmd_dev_handle *dev, uint16_t track,
                                      unsigned char* sessionkey)
{
    unsigned char cmdhdr[] = {0x00, 0x10, 0x01};
    unsigned char hash[8] = { 0 };
    unsigned char cmd[sizeof(cmdhdr) + sizeof(track) + sizeof(hash)];
    unsigned char *buf;

    int des_error;
    netmd_response response;
    netmd_error error;

    des_error = ecb_crypt((char*)sessionkey, (char*)hash, sizeof(hash), DES_ENCRYPT);
    if (DES_FAILED(des_error)) {
        return NETMD_DES_ERROR;
    }

    buf = cmd;
    memcpy(buf, cmdhdr, sizeof(cmdhdr));
    buf += sizeof(cmdhdr);
    netmd_copy_word_to_buffer(&buf, track);

    memcpy(buf, hash, sizeof(hash));
    buf += sizeof(hash);

    error = exch_secure_msg(dev, 0x48, cmd, sizeof(cmd), &response);
    netmd_check_response_bulk(&response, cmdhdr, sizeof(cmdhdr), &error);
    netmd_check_response_word(&response, track, &error);

    return error;
}

netmd_error netmd_secure_get_track_uuid(netmd_dev_handle *dev, uint16_t track,
                                        unsigned char *uuid)
{
    unsigned char cmdhdr[] = {0x10, 0x01};
    unsigned char cmd[sizeof(cmdhdr) + sizeof(track)];

    netmd_response response;
    netmd_error error;

    memcpy(cmd, cmdhdr, sizeof(cmdhdr));

    uint16_t tmp = track >> 8;
    cmd[sizeof(cmdhdr)] = tmp & 0xffU;
    cmd[sizeof(cmdhdr) + 1] = track & 0xffU;

    error = exch_secure_msg(dev, 0x23, cmd, sizeof(cmd), &response);
    netmd_check_response_bulk(&response, cmd, sizeof(cmd), &error);
    netmd_read_response_bulk(&response, uuid, 8, &error);

    return error;
}

netmd_error netmd_secure_delete_track(netmd_dev_handle *dev, uint16_t track,
                                      unsigned char *signature)
{
    unsigned char cmdhdr[] = {0x00, 0x10, 0x01};
    unsigned char cmd[sizeof(cmdhdr) + sizeof(track)];

    netmd_response response;
    netmd_error error;

    memcpy(cmd, cmdhdr, sizeof(cmdhdr));
    cmd_length += sizeof(cmdhdr);

    uint16_t tmp = track >> 8;
    cmd[sizeof(cmdhdr)] = tmp & 0xffU;
    cmd[sizeof(cmdhdr) + 1] = track & 0xffU;

    error = exch_secure_msg(dev, 0x40, cmd, sizeof(cmd), &response);
    netmd_check_response_bulk(&response, cmd, sizeof(cmd), &error);
    netmd_read_response_bulk(&response, signature, 8, &error);

    return error;
}
