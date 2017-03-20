/* netmd.c
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

void print_disc_info(netmd_dev_handle* devh, minidisc *md);
void print_current_track_info(netmd_dev_handle* devh);
void print_syntax();
void import_m3u_playlist(netmd_dev_handle* devh, const char *file);

/* Max line length we support in M3U files... should match MD TOC max */
#define M3U_LINE_MAX	128

/* Min "usable" audio file size (1 frame Atrac LP4)
   = 52 (RIFF/WAVE header Atrac LP) + 8 ("data" + length) + 92 (1 frame LP4) */
#define MIN_WAV_LENGTH 152

/*
static void handle_secure_cmd(netmd_dev_handle* devh, int cmdid, int track)
{
    unsigned int player_id;
    unsigned char ekb_head[] = {
        0x01, 0xca, 0xbe, 0x07, 0x2c, 0x4d, 0xa7, 0xae,
        0xf3, 0x6c, 0x8d, 0x73, 0xfa, 0x60, 0x2b, 0xd1};
    unsigned char ekb_body[] = {
        0x0f, 0xf4, 0x7d, 0x45, 0x9c, 0x72, 0xda, 0x81,
        0x85, 0x16, 0x9d, 0x73, 0x49, 0x00, 0xff, 0x6c,
        0x6a, 0xb9, 0x61, 0x6b, 0x03, 0x04, 0xf9, 0xce};
    unsigned char rand_in[8], rand_out[8];
    unsigned char hash8[8];
    unsigned char hash32[32];

    switch (cmdid) {
    case 0x11:
        if (netmd_secure_cmd_11(devh, &player_id) > 0) {
            fprintf(stdout, "Player id = %04d\n", player_id);
        }
        break;
    case 0x12:
        netmd_secure_cmd_12(devh, ekb_head, ekb_body);
        break;
    case 0x20:
        memset(rand_in, 0, sizeof(rand_in));
        if (netmd_secure_cmd_20(devh, rand_in, rand_out) > 0) {
            fprintf(stdout, "Random =\n");
            print_hex(rand_out, sizeof(rand_out));
        }
        break;
    case 0x21:
        netmd_secure_cmd_21(devh);
        break;
    case 0x22:
        memset(hash32, 0, sizeof(hash32));
        netmd_secure_cmd_22(devh, hash32);
        break;
    case 0x23:
        if (netmd_secure_cmd_23(devh, track, hash8) > 0) {
            fprintf(stdout, "Hash id of track %d =\n", track);
            print_hex(hash8, sizeof(hash8));
        }
        break;*/
/*case 0x28: TODO*/
        /*case 0x40:
        if (netmd_secure_cmd_40(devh, track, hash8) > 0) {
            fprintf(stdout, "Signature of deleted track %d =\n", track);
            print_hex(hash8, sizeof(hash8));
        }
        break;
    case 0x48:
        memset(hash8, 0, sizeof(hash8));
        if (netmd_secure_cmd_48(devh, track, hash8) > 0) {
            fprintf(stdout, "Signature of downloaded track %d =\n", track);
            print_hex(hash8, sizeof(hash8));
        }
        break;
    case 0x80:
        netmd_secure_cmd_80(devh);
        break;
    case 0x81:
        netmd_secure_cmd_81(devh);
        break;
    default:
        fprintf(stderr, "unsupported secure command\n");
        break;
    }
}
*/

static void send_raw_message(netmd_dev_handle* devh, char *pszRaw)
{
    unsigned char cmd[255], rsp[255];
    unsigned int data;
    char szBuf[4];
    size_t cmdlen;
    int rsplen;

    /* check raw message length */
    if ((strlen(pszRaw) % 2) != 0) {
        printf("Error: invalid length of raw message!\n");
        return;
    }

    /* convert hex message to bin */
    cmdlen = 0;
    while (*pszRaw != 0) {
        szBuf[0] = *pszRaw++;
        szBuf[1] = *pszRaw++;
        szBuf[2] = '\0';
        if (sscanf(szBuf, "%02X", &data) != 1) {
            printf("Error: invalid character at byte %lu ('%s')\n", (unsigned long)cmdlen, szBuf);
            return;
        }
        cmd[cmdlen++] = data & 0xff;
    }

    /* send it */
    rsplen = netmd_exch_message(devh, cmd, cmdlen, rsp);
    if (rsplen < 0) {
        printf("Error: netmd_exch_message failed with %d\n", rsplen);
        return;
    }
}

void print_time(const netmd_time *time)
{
    printf("%02d:%02d:%02d.%02d", time->hour, time->minute, time->second, time->frame);
}

void retailmac(unsigned char *rootkey, unsigned char *hostnonce,
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
    return (c[3] << 24) + (c[2] << 16) + (c[1] << 8) + c[0];
}

static inline unsigned int leword16(const unsigned char * c)
{
    return c[1]*256+c[0];
}

static int wav_data_position(const unsigned char * data, size_t offset, size_t len)
{
    int pos = -1, i = offset;

    while (i < len - 4) {
        if(strncmp("data", data+i, 4) == 0) {
            pos = i;
            break;
        }
        i += 2;
    }
    return pos;

}

static int audio_supported(const unsigned char * file, netmd_wireformat * wireformat, unsigned char * diskformat, int * conversion, size_t * channels, size_t * headersize)
{
    if(strncmp("RIFF", file, 4) != 0 || strncmp("WAVE", file+8, 4) != 0 || strncmp("fmt ", file+12, 4) != 0)
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

int main(int argc, char* argv[])
{
    netmd_dev_handle* devh;
    minidisc my_minidisc, *md = &my_minidisc;
    netmd_device *device_list, *netmd;
    unsigned int i = 0;
    unsigned int j = 0;
    char name[16];
    uint16_t track, playmode;
    int c;
    netmd_time time;
    netmd_error error;
    FILE *f;

    error = netmd_init(&device_list);
    if (error != NETMD_NO_ERROR) {
        printf("Error initializing netmd\n%s\n", netmd_strerror(error));
        return -1;
    }

    if (device_list == NULL) {
        puts("Found no NetMD device(s).");
        return -1;
    }

    /* pick first available device */
    netmd = device_list;

    error = netmd_open(netmd, &devh);
    if(error != NETMD_NO_ERROR)
    {
        printf("Error opening netmd\n%s\n", netmd_strerror(error));
        return -1;
    }

    error = netmd_get_devname(devh, name, 16);
    if (error != NETMD_NO_ERROR)
    {
        printf("Could not get device name\n%s\n", netmd_strerror(error));
        return -1;
    }
    printf("%s\n", name);

    netmd_initialize_disc_info(devh, md);
    printf("Disc Title: %s\n\n", md->groups[0].name);

    /* by default, log only errors */
    netmd_set_log_level(NETMD_LOG_ERROR);

    /* parse options */
    while (1) {
        c = getopt(argc, argv, "tv");
        if (c == -1) {
            break;
        }
        switch (c) {
        case 't':
            netmd_set_log_level(NETMD_LOG_ALL);
            break;
        case 'v':
            netmd_set_log_level(NETMD_LOG_VERBOSE);
            break;
        default:
            fprintf(stderr, "Unknown option '%c'\n", c);
            break;
        }
    }

    /* update argv and argc after parsing options */
    argv = &argv[optind - 1];
    argc -= (optind - 1);

    /* parse commands */
    if(argc > 1)
    {
        if(strcmp("rename", argv[1]) == 0)
        {
            i = strtoul(argv[2], NULL, 10);
            netmd_cache_toc(devh);
            netmd_set_title(devh, i & 0xffff, argv[3]);
            netmd_sync_toc(devh);
        }
        else if(strcmp("move", argv[1]) == 0)
        {
            i = strtoul(argv[2], NULL, 10);
            j = strtoul(argv[3], NULL, 10);
            netmd_move_track(devh, i & 0xffff, j & 0xffff);
        }
        else if(strcmp("groupmove", argv[1]) == 0)
        {
            i = strtoul(argv[2], NULL, 10);
            j = strtoul(argv[3], NULL, 10);
            netmd_move_group(devh, md, j & 0xffff, i & 0xffff);
        }
        else if(strcmp("write", argv[1]) == 0)
        {
            if(netmd_write_track(devh, argv[2]) < 0)
            {
                fprintf(stderr, "Error writing track %i\n", errno);
            }
        }
        else if(strcmp("newgroup", argv[1]) == 0)
        {
            netmd_create_group(devh, md, argv[2]);
        }
        else if(strcmp("settitle", argv[1]) == 0)
        {
            netmd_cache_toc(devh);
            netmd_set_disc_title(devh, argv[2], strlen(argv[2]));
            netmd_sync_toc(devh);
        }
        else if(strcmp("group", argv[1]) == 0)
        {
            i = strtoul(argv[2], NULL, 10);
            j = strtoul(argv[3], NULL, 10);
            if(!netmd_put_track_in_group(devh, md, i & 0xffff, j & 0xffff))
            {
                printf("Something screwy happened\n");
            }
        }
        else if(strcmp("retitle", argv[1]) == 0)
        {
            i = strtoul(argv[2], NULL, 10);
            netmd_set_group_title(devh, md, i, argv[3]);
        }
        else if(strcmp("play", argv[1]) == 0)
        {
            if( argc > 2 ) {
                i = strtoul(argv[2],NULL, 10);
                netmd_set_track( devh, i & 0xffff );
            }
            netmd_play(devh);
        }
        else if(strcmp("stop", argv[1]) == 0)
        {
            netmd_stop(devh);
        }
        else if(strcmp("pause", argv[1]) == 0)
        {
            netmd_pause(devh);
        }
        else if(strcmp("fforward", argv[1]) == 0)
        {
            netmd_fast_forward(devh);
        }
        else if(strcmp("rewind", argv[1]) == 0)
        {
            netmd_rewind(devh);
        }
        else if(strcmp("next", argv[1]) == 0)
        {
            netmd_track_next(devh);
        }
        else if(strcmp("previous", argv[1]) == 0)
        {
            netmd_track_previous(devh);
        }
        else if(strcmp("restart", argv[1]) == 0)
        {
            netmd_track_restart(devh);
        }
        else if(strcmp("settime", argv[1]) == 0)
        {
            track = strtoul(argv[2], (char **) NULL, 10) & 0xffff;
            if (argc > 6)
            {
                time.hour = strtoul(argv[3], (char **) NULL, 10) & 0xffff;
                time.minute = strtoul(argv[4], (char **) NULL, 10) & 0xff;
                time.second = strtoul(argv[5], (char **) NULL, 10) & 0xff;
                time.frame = strtoul(argv[6], (char **) NULL, 10) & 0xff;
            }
            else
            {
                time.hour = 0;
                time.minute = strtoul(argv[3], (char **) NULL, 10) & 0xff;
                time.second = strtoul(argv[4], (char **) NULL, 10) & 0xff;
                if (argc > 5)
                {
                    time.frame = strtoul(argv[5], (char **) NULL, 10) & 0xff;;
                }
                else
                {
                    time.frame = 0;
                }
            }

            netmd_set_time(devh, track, &time);
        }
        else if(strcmp("m3uimport", argv[1]) == 0)
        {
            import_m3u_playlist(devh, argv[2]);
        }
        else if(strcmp("delete", argv[1]) == 0)
        {
            i = strtoul(argv[2], NULL, 10);
            netmd_delete_track(devh, i & 0xffff);
        }
        else if(strcmp("deletegroup", argv[1]) == 0)
        {
            i = strtoul(argv[2], NULL, 10);
            netmd_delete_group(devh, md, i & 0xffff);
        }
        else if(strcmp("status", argv[1]) == 0) {
            print_current_track_info(devh);
        }
        else if (strcmp("raw", argv[1]) == 0) {
            send_raw_message(devh, argv[2]);
        }
        else if (strcmp("setplaymode", argv[1]) == 0) {
            playmode = 0;
            int i;
            for (i = 2; i < argc; i++) {
                if (strcmp(argv[i], "single") == 0) {
                    playmode |= NETMD_PLAYMODE_SINGLE;
                }
                else if (strcmp(argv[i], "repeat") == 0) {
                    playmode |= NETMD_PLAYMODE_REPEAT;
                }
                else if (strcmp(argv[i], "shuffle") == 0) {
                    playmode |= NETMD_PLAYMODE_SHUFFLE;
                }
            }
            printf("%x\n", playmode);
            netmd_set_playmode(devh, playmode);
        }
        else if (strcmp("capacity", argv[1]) == 0) {
            netmd_disc_capacity capacity;
            netmd_get_disc_capacity(devh, &capacity);

            printf("Recorded:  ");
            print_time(&capacity.recorded);
            printf("\nTotal:     ");
            print_time(&capacity.total);
            printf("\nAvailable: ");
            print_time(&capacity.available);
            printf("\n");
        }
        else if (strcmp("recv", argv[1]) == 0) {
            i = strtoul(argv[2], NULL, 10);
            f = fopen(argv[3], "wb");
            netmd_secure_recv_track(devh, i & 0xffff, f);
            fclose(f);
        }
        else if (strcmp("send", argv[1]) == 0) {
            netmd_error error;
                    netmd_ekb ekb;
                    unsigned char chain[] = {0x25, 0x45, 0x06, 0x4d, 0xea, 0xca,
                                             0x14, 0xf9, 0x96, 0xbd, 0xc8, 0xa4,
                                             0x06, 0xc2, 0x2b, 0x81, 0x49, 0xba,
                                             0xf0, 0xdf, 0x26, 0x9d, 0xb7, 0x1d,
                                             0x49, 0xba, 0xf0, 0xdf, 0x26, 0x9d,
                                             0xb7, 0x1d};
                    unsigned char signature[] = {0xe8, 0xef, 0x73, 0x45, 0x8d, 0x5b,
                                                 0x8b, 0xf8, 0xe8, 0xef, 0x73, 0x45,
                                                 0x8d, 0x5b, 0x8b, 0xf8, 0x38, 0x5b,
                                                 0x49, 0x36, 0x7b, 0x42, 0x0c, 0x58};
                    unsigned char rootkey[] = {0x13, 0x37, 0x13, 0x37, 0x13, 0x37,
                                               0x13, 0x37, 0x13, 0x37, 0x13, 0x37,
                                               0x13, 0x37, 0x13, 0x37};
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
                    struct stat stat_buf;
                    unsigned char *data = NULL;
                    size_t data_size;

                    uint16_t track;
                    unsigned char uuid[8] = { 0 };
                    unsigned char new_contentid[20] = { 0 };
                    char title[256] = {0};

                    size_t frames, headersize, channels;
                    int data_position, audio_data_position, audio_data_size, i, need_conversion = 1, file_valid = 0;
                    unsigned char * audio_data;
                    netmd_wireformat wireformat;
                    unsigned char discformat;

                    /* read source */
                    stat(argv[2], &stat_buf);
                    if((data_size = (size_t)stat_buf.st_size) < MIN_WAV_LENGTH)
                        netmd_log(NETMD_LOG_ERROR, "audio file too small (corrupt or not supported)\n");
                    else
                        file_valid = 1;
                    netmd_log(NETMD_LOG_VERBOSE, "audio file size : %d bytes\n", data_size );

                    /* open audio file */
                    if(file_valid) {
                        if((data = (unsigned char *)malloc(data_size)) == NULL) {
                            file_valid = 0;
                            netmd_log(NETMD_LOG_ERROR, "error allocating memory for file input\n" );
                        }
                        else {
                            if(!(f = fopen(argv[2], "rb"))) {
                                netmd_log(NETMD_LOG_ERROR, "cannot open audio file\n" );
                                file_valid = 0;
                                free(data);
                            }
                        }
                    }

                    /* copy file to buffer */
                    if(file_valid) {
                        if((fread(data, data_size, 1, f)) < 1) {
                            netmd_log(NETMD_LOG_ERROR, "cannot read audio file\n" );
                            file_valid = 0;
                            free(data);
                        }
                        fclose(f);
                    }

                    /* check contents */
                    if(file_valid) {
                        file_valid = audio_supported(data, &wireformat, &discformat, &need_conversion, &channels, &headersize );
                        if(!file_valid) {
                            netmd_log(NETMD_LOG_ERROR, "audio file unknown or not supported\n" );
                            free(data);
                        }
                        else {
                            netmd_log(NETMD_LOG_VERBOSE, "supported audio file detected\n");
                            if((data_position = wav_data_position(data, headersize, data_size)) < 0) {
                                netmd_log(NETMD_LOG_VERBOSE, "cannot locate audio data in file\n" );
                                free(data);
                                file_valid = 0;
                            }
                            else {
                                netmd_log(NETMD_LOG_VERBOSE, "data chunk position at %d\n", data_position);
                                audio_data_position = data_position+8;
                                audio_data = data+audio_data_position;
                                audio_data_size = leword32(data+(data_position+4));
                                netmd_log(NETMD_LOG_VERBOSE, "audio data size read from file :           %d bytes\n", audio_data_size);
                                netmd_log(NETMD_LOG_VERBOSE, "audio data size calculated from file size: %d bytes\n", data_size - audio_data_position);

                            }
                        }
                    }

                    if(file_valid)
                    {
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
                        for (done = 0; done < sizeof(chain); done+=16U)
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
                        if(need_conversion)
                        {
                            for(i = 0; i < audio_data_size; i+=2)
                            {
                                unsigned char first = audio_data[i];
                                audio_data[i] = audio_data[i+1];
                                audio_data[i+1] = first;
                            }
                        }

                        /* number of frames will be calculated by netmd_prepare_packets() depending on the wire format and channels */
                        error = netmd_prepare_packets(audio_data, audio_data_size, &packets, &packet_count, &frames, channels, kek, wireformat);
                        netmd_log(NETMD_LOG_VERBOSE, "netmd_prepare_packets : %s\n", netmd_strerror(error));

                        /* send to device */
                        error = netmd_secure_send_track(devh, wireformat,
                                                        discformat,
                                                        frames, packets,
                                                        packet_count, sessionkey,
                                                        &track, uuid, new_contentid);
                        netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_send_key_data : %s\n", netmd_strerror(error));

                        /* cleanup */
                        netmd_cleanup_packets(&packets);
                        free(data);
                        audio_data = NULL;

                        /* set title, use filename */
                        memcpy(title, argv[2], strlen(argv[2])-4);
                        netmd_log(NETMD_LOG_VERBOSE, "New Track: %d\n", track);
                        netmd_cache_toc(devh);
                        netmd_set_title(devh, track, title);
                        netmd_sync_toc(devh);

                        /* commit track */
                        error = netmd_secure_commit_track(devh, track, sessionkey);
                        netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_commit_track : %s\n", netmd_strerror(error));

                        /* forget key */
                        error = netmd_secure_session_key_forget(devh);
                        netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_session_key_forget : %s\n", netmd_strerror(error));

                        /* leave session */
                        error = netmd_secure_leave_session(devh);
                        netmd_log(NETMD_LOG_VERBOSE, "netmd_secure_leave_session : %s\n", netmd_strerror(error));
                    }
        }
        else if(strcmp("help", argv[1]) == 0)
        {
            print_syntax();
        }
        else
        {
            print_disc_info(devh, md);
            print_syntax();
        }
    }
    else
        print_disc_info(devh, md);

    netmd_clean_disc_info(md);
    netmd_close(devh);
    netmd_clean(&device_list);

    return 0;
}

void print_current_track_info(netmd_dev_handle* devh)
{
    uint16_t track;
    char buffer[256];
    netmd_time time;

    /* TODO: error checking */
    netmd_get_position(devh, &time);
    netmd_get_track(devh, &track);
    netmd_request_title(devh, track, buffer, 256);

    printf("Current track: %s \n", buffer);
    printf("Current playback position: ");
    print_time(&time);
    printf("\n");

}

void print_disc_info(netmd_dev_handle* devh, minidisc* md)
{
    uint8_t i = 0;
    int size = 1;
    uint8_t g, group = 0, lastgroup = 0;
    unsigned char bitrate_id;
    unsigned char flags;
    unsigned char channel;
    char *name, buffer[256];
    struct netmd_track time;
    struct netmd_pair const *trprot, *bitrate;

    trprot = bitrate = 0;

    for(i = 0; size >= 0; i++)
    {
        size = netmd_request_title(devh, i, buffer, 256);

        if(size < 0)
        {
            break;
        }

        /* Figure out which group this track is in */
        for( group = 0, g = 1; g < md->group_count; g++ )
        {
            if( (md->groups[g].start <= i+1U) && (md->groups[g].finish >= i+1U ))
            {
                group = g;
                break;
            }
        }
        /* Different to the last group? */
        if( group != lastgroup )
        {
            lastgroup = group;
            if( group )			/* Group 0 is 'no group' */
            {
                printf("Group: %s\n", md->groups[group].name);
            }
        }
        /* Indent tracks which are in a group */
        if( group )
        {
            printf("  ");
        }

        netmd_request_track_time(devh, i, &time);
        netmd_request_track_flags(devh, i, &flags);
        netmd_request_track_bitrate(devh, i, &bitrate_id, &channel);

        trprot = find_pair(flags, trprot_settings);
        bitrate = find_pair(bitrate_id, bitrates);

        /* Skip 'LP:' prefix... the codec type shows up in the list anyway*/
        if( strncmp( buffer, "LP:", 3 ))
        {
            name = buffer;
        }
        else
        {
            name = buffer + 3;
        }

        printf("Track %2i: %-6s %6s - %02i:%02i:%02i - %s\n",
               i, trprot->name, bitrate->name, time.minute,
               time.second, time.tenth, name);
    }

    /* XXX - This needs a rethink with the above method */
    /* groups may not have tracks, print the rest. */
    printf("\n--Empty Groups--\n");
    for(group=1; group < md->group_count; group++)
    {
        if(md->groups[group].start == 0 && md->groups[group].finish == 0) {
            printf("Group: %s\n", md->groups[group].name);
        }

    }

    printf("\n\n");
}

void import_m3u_playlist(netmd_dev_handle* devh, const char *file)
{
    FILE *fp;
    char buffer[M3U_LINE_MAX + 1];
    char *s;
    uint8_t track;
    int discard;

    if( file == NULL )
    {
        printf( "No filename specified\n" );
        print_syntax();
        return;
    }

    if( (fp = fopen( file, "r" )) == NULL )
    {
        printf( "Unable to open file %s: %s\n", file, strerror( errno ));
        return;
    }

    if( ! fgets( buffer, M3U_LINE_MAX, fp )) {
        printf( "File Read error\n" );
        return;
    }
    if( strcmp( buffer, "#EXTM3U\n" )) {
        printf( "Invalid M3U playlist\n" );
        return;
    }

    track = 0;
    discard = 0;
    while( fgets( buffer, M3U_LINE_MAX, fp) != NULL ) {
        /* Chomp newlines */
        s = strchr( buffer, '\n' );
        if( s )
            *s = '\0';

        if( buffer[0] == '#' )
        {
            /* comment, ext3inf etc... we only care about ext3inf */
            if( strncmp( buffer, "#EXTINF:", 8 ))
            {
                printf( "Skip: %s\n", buffer );
            }
            else
            {
                s = strchr( buffer, ',' );
                if( !s )
                {
                    printf( "M3U Syntax error! %s\n", buffer );
                }
                else
                {
                    s++;
                    printf( "Title track %d - %s\n", track, s );
                    netmd_set_title(devh, track, s); /* XXX Handle errors */
                    discard = 1;	/* don't fallback to titling by filename */
                }
            }
        }
        else
        {
            /* Filename line */
            if( discard )
            {
                /* printf( "Discard: %s\n", buffer ); */
                discard = 0;
            }
            else
            {
                /* Try and generate a title from the track name */
                s = strrchr( buffer, '.' ); /* Isolate extension */
                if( s )
                    *s = 0;
                s = strrchr( buffer, '/' ); /* Isolate basename */
                if( !s )
                    s = strrchr( buffer, '\\' ); /* Handle DOS paths? */
                if( !s )
                    s = buffer;
                else
                    s++;

                printf( "Title track %d - %s\n", track, s );
                netmd_set_title(devh, track, s); /* XXX Handle errors */
            }
            track++;
        }
    }
}


void print_syntax()
{
    puts("\nNetMD test suite.");
    puts("Usage: netmd [options] command args");
    puts("Options:");
    puts("      -v show debug messages");
    puts("      -t enable tracing of USB command and response data");
    puts("Commands:");
    puts("rename # <string> - rename track # to <string> track numbers are off by one (ie track 1 is 0)");
    puts("move #1 #2 - make track #1 track #2");
    puts("groupmove #1 #2 - make group #1 start at track #2 !BUGGY!");
    puts("deletegroup #1 - delete a group, but not the tracks in it");
    puts("write <string> - write omg file to netmd unit !DOES NOT WORK!");
    puts("group #1 #2 - Stick track #1 into group #2");
    puts("retitle #1 <string> - rename group number #1 to <string>");
    puts("play #1 - play track #");
    puts("fforward - start fast forwarding");
    puts("rewind - start rewinding");
    puts("next - starts next track");
    puts("previous - starts previous track");
    puts("restart - restarts current track");
    puts("pause - pause the unit");
    puts("stop - stop the unit");
    puts("delete #1 - delete track");
    puts("m3uimport - import playlist - and title current disc using it.");
    puts("send #1 - send wav audio file #1 to the device");
    puts("          #1 supported files: 16 bit pcm (stere or mono) @44100Hz or");
    puts("          Atrac LP2/LP4 stored in a wav container");
    puts("raw - send raw command (hex)");
    puts("setplaymode (single, repeat, shuffle) - set play mode");
    puts("newgroup <string> - create a new group named <string>");
    puts("settitle <string> - manually set the complete disc title (with group information)");
    puts("settime <track> [<hour>] <minute> <second> [<frame>] - seeks to the given timestamp");
    puts("      (if three values are given, they are minute, second and frame)");
    puts("secure #1 #2 - execute secure command #1 on track #2 (where applicable)");
    puts("  --- general ---");
    puts("  0x80 = start secure session");
    puts("  0x11 = get player id");
    puts("  0x12 = send ekb");
    puts("  0x20 = exchange randoms");
    puts("  0x21 = discard randoms");
    puts("  0x81 = end secure session");
    puts("  --- check-out ---");
    puts("  0x22 = submit 32-byte hash");
    puts("  0x28 = prepare download");
    puts("  0x48 = verify downloaded track #");
    puts("  --- check-in ---");
    puts("  0x23 = get hash id for track #");
    puts("  0x40 = secure delete track #");
    puts("help - print this stuff");
}
