/* send.c -- split off from netmdcli.c
 *      Copyright (C) 2017 René Rebe
 *      Copyright (C) 2002, 2003 Marc Britten
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

#include <gcrypt.h>
#include <unistd.h>

#include "libnetmd.h"
#include "utils.h"

/* Min "usable" audio file size (1 frame Atrac LP4)
   = 52 (RIFF/WAVE header Atrac LP) + 8 ("data" + length) + 92 (1 frame LP4) */
#define MIN_WAV_LENGTH 152

static void retailmac(unsigned char *rootkey, unsigned char *hostnonce,
               unsigned char *devnonce, unsigned char *sessionkey)
{
    gcry_cipher_hd_t handle1;
    gcry_cipher_hd_t handle2;

    unsigned char des3_key[24] = { 0 };
    unsigned char iv[8] = { 0 };

    gcry_cipher_open(&handle1, GCRY_CIPHER_DES, GCRY_CIPHER_MODE_ECB, 0);
    gcry_cipher_setkey(handle1, rootkey, 8);
    gcry_cipher_encrypt(handle1, iv, 8, hostnonce, 8);

    memcpy(des3_key, rootkey, 16);
    memcpy(des3_key+16, rootkey, 8);
    gcry_cipher_open(&handle2, GCRY_CIPHER_3DES, GCRY_CIPHER_MODE_CBC, 0);
    gcry_cipher_setkey(handle2, des3_key, 24);
    gcry_cipher_setiv(handle2, iv, 8);
    gcry_cipher_encrypt(handle2, sessionkey, 8, devnonce, 8);

    gcry_cipher_close(handle1);
    gcry_cipher_close(handle2);
}

static inline unsigned int leword32(const unsigned char * c)
{
    return (unsigned int)((c[3] << 24U) + (c[2] << 16U) + (c[1] << 8U) + c[0]);
}

static inline unsigned int leword16(const unsigned char * c)
{
    return c[1]*256U+c[0];
}

static size_t wav_data_position(const unsigned char * data, size_t offset, size_t len)
{
    size_t i = offset, pos = 0;

    while (i < len - 4) {
        if(strncmp("data", (const char *)data+i, 4) == 0) {
            pos = i;
            break;
        }
        i += 2;
    }

    return pos;
}

static int audio_supported(const unsigned char * file, netmd_wireformat * wireformat, unsigned char * diskformat, int * conversion, size_t * channels, size_t * headersize)
{
    if(strncmp("RIFF", (const char *)file, 4) != 0 || strncmp("WAVE", (const char *)file+8, 4) != 0 || strncmp("fmt ", (const char *)file+12, 4) != 0)
        return 0;                                             /* no valid WAV file or fmt chunk missing*/

    if(leword16(file+20) == 1)                                /* PCM */
    {
        *conversion = 1;                                      /* needs conversion (byte swapping) for pcm raw data from wav file*/
        *wireformat = NETMD_WIREFORMAT_PCM;
        if(leword32(file+24) != 44100)                        /* sample rate not 44k1*/
            return 0;
        if(leword16(file+34) != 16)                           /* bitrate not 16bit */
            return 0;
        if(leword16(file+22) == 2) {                          /* channels = 2, stereo */
            *channels = NETMD_CHANNELS_STEREO;
            *diskformat = NETMD_DISKFORMAT_SP_STEREO;
        }
        else if(leword16(file+22) == 1) {                     /* channels = 1, mono */
            *channels = NETMD_CHANNELS_MONO;
            *diskformat = NETMD_DISKFORMAT_SP_MONO;
        }
        else
            return 0;
        *headersize = 20 + leword32(file+16);
        return 1;
    }

    if(leword16(file +20) == NETMD_RIFF_FORMAT_TAG_ATRAC3)         /* ATRAC3 */
    {
        *conversion = 0;                                           /* conversion not needed */
        if(leword32(file+24) != 44100)                             /* sample rate */
            return 0;
        if(leword16(file+32) == NETMD_DATA_BLOCK_SIZE_LP2) {       /* data block size LP2 */
            *wireformat = NETMD_WIREFORMAT_LP2;
            *diskformat = NETMD_DISKFORMAT_LP2;
        }
        else if(leword16(file+32) == NETMD_DATA_BLOCK_SIZE_LP4) {  /* data block size LP4 */
            *wireformat = NETMD_WIREFORMAT_LP4;
            *diskformat = NETMD_DISKFORMAT_LP4;
        }
        else
            return 0;
        *headersize = 20 + leword32(file+16);
        *channels = NETMD_CHANNELS_STEREO;
        return 1;
    }
    return 0;
}

netmd_error
netmd_send_track(netmd_dev_handle *devh, const char *filename, const char *in_title,
        netmd_send_progress_func send_progress, void *send_progress_user_data)
{
    netmd_error error;
    netmd_ekb ekb;
    unsigned char chain[] = { 0x25, 0x45, 0x06, 0x4d, 0xea, 0xca,
        0x14, 0xf9, 0x96, 0xbd, 0xc8, 0xa4,
        0x06, 0xc2, 0x2b, 0x81, 0x49, 0xba,
        0xf0, 0xdf, 0x26, 0x9d, 0xb7, 0x1d,
        0x49, 0xba, 0xf0, 0xdf, 0x26, 0x9d,
        0xb7, 0x1d };
    unsigned char signature[] = { 0xe8, 0xef, 0x73, 0x45, 0x8d, 0x5b,
        0x8b, 0xf8, 0xe8, 0xef, 0x73, 0x45,
        0x8d, 0x5b, 0x8b, 0xf8, 0x38, 0x5b,
        0x49, 0x36, 0x7b, 0x42, 0x0c, 0x58 };
    unsigned char rootkey[] = { 0x13, 0x37, 0x13, 0x37, 0x13, 0x37,
        0x13, 0x37, 0x13, 0x37, 0x13, 0x37,
        0x13, 0x37, 0x13, 0x37 };
    netmd_keychain *keychain;
    netmd_keychain *next;
    size_t done;
    unsigned char hostnonce[8] = { 0 };
    unsigned char devnonce[8] = { 0 };
    unsigned char sessionkey[8] = { 0 };
    unsigned char kek[] = { 0x14, 0xe3, 0x83, 0x4e, 0xe2, 0xd3, 0xcc, 0xa5 };
    unsigned char contentid[] = { 0x01, 0x0F, 0x50, 0x00, 0x00, 0x04,
        0x00, 0x00, 0x00, 0x48, 0xA2, 0x8D,
        0x3E, 0x1A, 0x3B, 0x0C, 0x44, 0xAF,
        0x2f, 0xa0 };
    netmd_track_packets *packets = NULL;
    size_t packet_count = 0;
    size_t packet_length = 0;
    struct stat stat_buf;
    unsigned char *data = NULL;
    size_t data_size;
    FILE *f;

    uint16_t track;
    unsigned char uuid[8] = { 0 };
    unsigned char new_contentid[20] = { 0 };
    char title[256] = { 0 };

    size_t headersize, channels;
    unsigned int frames;
    size_t data_position, audio_data_position, audio_data_size, i;
    int need_conversion = 1/*, file_valid = 0*/;
    unsigned char * audio_data;
    netmd_wireformat wireformat;
    unsigned char discformat;

    /* read source */
    stat(filename, &stat_buf);
    if ((data_size = (size_t)stat_buf.st_size) < MIN_WAV_LENGTH) {
        netmd_log(NETMD_LOG_ERROR, "audio file too small (corrupt or not supported)\n");
        return NETMD_ERROR;
    }

    netmd_log(NETMD_LOG_VERBOSE, "audio file size : %d bytes\n", data_size);

    /* open audio file */
    if ((data = (unsigned char *)malloc(data_size + 2048)) == NULL) {      // reserve additional mem for padding if needed
        netmd_log(NETMD_LOG_ERROR, "error allocating memory for file input\n");
        return NETMD_ERROR;
    }
    else {
        if (!(f = fopen(filename, "rb"))) {
            netmd_log(NETMD_LOG_ERROR, "cannot open audio file\n");
            free(data);

            return NETMD_ERROR;
        }
    }

    /* copy file to buffer */
    memset(data, 0, data_size + 8);
    if ((fread(data, data_size, 1, f)) < 1) {
        netmd_log(NETMD_LOG_ERROR, "cannot read audio file\n");
        free(data);

        return NETMD_ERROR;
    }
    fclose(f);

    /* check contents */
    if (!audio_supported(data, &wireformat, &discformat, &need_conversion, &channels, &headersize)) {
        netmd_log(NETMD_LOG_ERROR, "audio file unknown or not supported\n");
        free(data);

        return NETMD_ERROR;
    }
    else {
        netmd_log(NETMD_LOG_VERBOSE, "supported audio file detected\n");
        if ((data_position = wav_data_position(data, headersize, data_size)) == 0) {
            netmd_log(NETMD_LOG_ERROR, "cannot locate audio data in file\n");
            free(data);

            return NETMD_ERROR;
        }
        else {
            netmd_log(NETMD_LOG_VERBOSE, "data chunk position at %d\n", data_position);
            audio_data_position = data_position + 8;
            audio_data = data + audio_data_position;
            audio_data_size = leword32(data + (data_position + 4));
            netmd_log(NETMD_LOG_VERBOSE, "audio data size read from file :           %d bytes\n", audio_data_size);
            netmd_log(NETMD_LOG_VERBOSE, "audio data size calculated from file size: %d bytes\n", data_size - audio_data_position);

        }
    }

    /* acquire device - needed by Sharp devices, may fail on Sony devices */
    error = netmd_acquire_dev(devh);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_acquire_dev: %s\n", netmd_strerror(error));

    error = netmd_secure_leave_session(devh);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_leave_session : %s\n", netmd_strerror(error));

    error = netmd_secure_set_track_protection(devh, 0x01);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_set_track_protection : %s\n", netmd_strerror(error));

    error = netmd_secure_enter_session(devh);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_enter_session : %s\n", netmd_strerror(error));

    /* build ekb */
    ekb.id = 0x26422642;
    ekb.depth = 9;
    ekb.signature = malloc(sizeof(signature));
    memcpy(ekb.signature, signature, sizeof(signature));

    /* build ekb key chain */
    ekb.chain = NULL;
    for (done = 0; done < sizeof(chain); done += 16U)
    {
        next = malloc(sizeof(netmd_keychain));
        if (ekb.chain == NULL) {
            ekb.chain = next;
        }
        else {
            keychain->next = next;
        }
        next->next = NULL;

        next->key = malloc(16);
        memcpy(next->key, chain + done, 16);

        keychain = next;
    }

    error = netmd_secure_send_key_data(devh, &ekb);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_send_key_data : %s\n", netmd_strerror(error));

    /* cleanup */
    free(ekb.signature);
    keychain = ekb.chain;
    while (keychain != NULL) {
        next = keychain->next;
        free(keychain->key);
        free(keychain);
        keychain = next;
    }

    /* exchange nonces */
    gcry_create_nonce(hostnonce, sizeof(hostnonce));
    error = netmd_secure_session_key_exchange(devh, hostnonce, devnonce);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_session_key_exchange : %s\n", netmd_strerror(error));

    /* calculate session key */
    retailmac(rootkey, hostnonce, devnonce, sessionkey);

    error = netmd_secure_setup_download(devh, contentid, kek, sessionkey);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_setup_download : %s\n", netmd_strerror(error));

    /* conversion (byte swapping) for pcm raw data from wav file if needed */
    if (need_conversion)
    {
        for (i = 0; i < audio_data_size; i += 2)
        {
            unsigned char first = audio_data[i];
            audio_data[i] = audio_data[i + 1];
            audio_data[i + 1] = first;
        }
    }

    /* number of frames will be calculated by netmd_prepare_packets() depending on the wire format and channels */
    error = netmd_prepare_packets(audio_data, audio_data_size, &packets, &packet_count, &frames, channels, &packet_length, kek, wireformat);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_prepare_packets : %s\n", netmd_strerror(error));

    /* send to device */
    error = netmd_secure_send_track(devh, wireformat,
        discformat,
        frames, packets,
        packet_length, sessionkey,
        &track, uuid, new_contentid,
        send_progress, send_progress_user_data);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_send_track : %s\n", netmd_strerror(error));

    /* cleanup */
    netmd_cleanup_packets(&packets);
    free(data);
    audio_data = NULL;

    if (error == NETMD_NO_ERROR) {
        char *titlep = title;

        /* set title, use either user-specified title or filename */
        if (in_title != NULL)
            strncpy(title, in_title, sizeof(title) - 1);
        else {
            strncpy(title, filename, sizeof(title) - 1);

            /* eliminate file extension */
            char *ext_dot = strrchr(title, '.');
            if (ext_dot != NULL)
                *ext_dot = '\0';

            /* eliminate path */
            char *title_slash = strrchr(title, '/');
            if (title_slash != NULL)
                titlep = title_slash + 1;
        }

        netmd_log(NETMD_LOG_VERBOSE, "New Track: %d\n", track);
        netmd_set_track_title(devh, track, titlep);

        /* commit track */
        error = netmd_secure_commit_track(devh, track, sessionkey);
        if (error == NETMD_NO_ERROR)
            netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_commit_track : %s\n", netmd_strerror(error));
        else
            netmd_log(NETMD_LOG_ERROR, "netmd_secure_commit_track failed : %s\n", netmd_strerror(error));
    }
    else {
        netmd_log(NETMD_LOG_ERROR, "netmd_secure_send_track failed : %s\n", netmd_strerror(error));
    }

    /* forget key */
    netmd_error cleanup_error = netmd_secure_session_key_forget(devh);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_session_key_forget : %s\n", netmd_strerror(cleanup_error));

    /* leave session */
    cleanup_error = netmd_secure_leave_session(devh);
    netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_leave_session : %s\n", netmd_strerror(cleanup_error));

    /* release device - needed by Sharp devices, may fail on Sony devices */
    int release_error = netmd_release_dev(devh);
    if (release_error <= 0) {
        netmd_log(NETMD_LOG_VERBOSE, "netmd_release_dev : %d\n", release_error);
    }

    return error; /* return error code from the "business logic" */
}
