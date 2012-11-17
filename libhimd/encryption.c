/*
 * encryption.c
 *
 * This file is part of libhimd, a library for accessing Sony HiMD devices.
 *
 * Copyright (C) 2009-2011 Michael Karcher
 * Copyright (C) 2011 Mårten Cassel
 * Copyright (C) 2011 Thomas Arp
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "himd.h"
#include <stdlib.h>

#define _(x) (x)

/** 
 * Calculate the key for a given MP3 track and discid of HiMD data.
 * The key is required to encrypt or decrypt MP3 data and is calculated
 * with a fairly simple algorithm which XORs parts of the discid with
 * two constant hashes and the track number.
 * 
 * @param himd Pointer to a descriptor of previously opened HiMD data
 * @param track Number of track to calculate MP3 key for
 * @param key Pointer to struct containing the key after successful operation
 * @param status Pointer to himderrinfo, returns error code after operation
 * 
 * @return Returns 0 if successful, otherwise zero.
 */
int himd_obtain_mp3key(struct himd * himd, int track, mp3key * key, struct himderrinfo * status)
{
    const unsigned char * d = himd_get_discid(himd, status);
    unsigned int foo;
    if(!d)
        return -1;
       
    foo = ((track*0x6953B2ED)+0x6BAAB1) ^
                 ((d[12] << 24) | (d[13] << 16) | (d[14] << 8) | d[15]) ;
    (*key)[0] = foo >> 24;
    (*key)[1] = foo >> 16;
    (*key)[2] = foo >> 8;
    (*key)[3] = foo;
    return 0;
}

#ifdef CONFIG_WITH_GCRYPT
#include "himd_private.h"
#include <gcrypt.h>
#include <string.h>

struct cached_cipher {
    unsigned char key[8];
    gcry_cipher_hd_t cipher;
    int valid;
};

struct descrypt_data {
    struct cached_cipher master;
    struct cached_cipher block;
    unsigned char masterkey[8];
};

static gcry_error_t cached_cipher_init(struct cached_cipher * cipher, enum gcry_cipher_modes mode)
{
    gcry_error_t err;
    err = gcry_cipher_open(&cipher->cipher, GCRY_CIPHER_DES, mode, 0);
    if (err != 0)
        return err;

    cipher->valid = 0;
    return 0;
}

/* iv should be NULL for ECB mode */
static gcry_error_t cached_cipher_prepare(struct cached_cipher * cipher,
                                 unsigned char * key, unsigned char * iv)
{
    gcry_error_t err;

    /* not yet initialized or new key */
    if(!cipher->valid ||
       memcmp(cipher->key, key, 8))
    {
        err = gcry_cipher_setkey(cipher->cipher, key, 8);
        if(err != 0)
            return err;

        memcpy(cipher->key, key, 8);
        cipher->valid = 1;
    }
    if(iv)
    {
        err = gcry_cipher_setiv(cipher->cipher, iv, 8);
        if(err != 0)
            return err;
    }
    return 0;
}

static void cached_cipher_deinit(struct cached_cipher * cipher)
{
    gcry_cipher_close(cipher->cipher);
}

static void xor_keys(unsigned char * out,
                     const unsigned char * in1, const unsigned char * in2)
{
    int i;
    for(i = 0; i < 8; i++)
        out[i] = in1[i] ^ in2[i];
}

int descrypt_open(void ** dataptr, const unsigned char * trackkey, 
                  unsigned int ekbnum, struct himderrinfo * status)
{
    /* gcrypt only supports three-key 3DES, so set key1 == key3 */
    static const unsigned char ekb00010012root[] = {0xf5,0x1e,0xcb,0x2a,0x80,0x8f,0x15,0xfd,
                                                    0x54,0x2e,0xf5,0x12,0x3b,0xcd,0xbc,0xa4,
                                                    0xf5,0x1e,0xcb,0x2a,0x80,0x8f,0x15,0xfd};
    gcry_cipher_hd_t rootcipher;
    struct descrypt_data * data;
    int err;

    if(ekbnum != 0x00010012)
    {
        set_status_const(status, HIMD_ERROR_UNSUPPORTED_ENCRYPTION, _("EKB %08x unsupported"));
        return -1;
    }

    data = malloc(sizeof *data);
    if(!data)
    {
        set_status_const(status, HIMD_ERROR_OUT_OF_MEMORY, _("Can't allocate crypt helper structure"));
        return -1;
    }
    if(gcry_cipher_open(&rootcipher, GCRY_CIPHER_3DES, GCRY_CIPHER_MODE_ECB, 0) != 0)
    {
        set_status_const(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't aquire 3DES ECB encryption"));
        return -1;
    }
    if((err = gcry_cipher_setkey(rootcipher, ekb00010012root, 24)) != 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't init 3DES: %s"), gcry_strerror(err));
        gcry_cipher_close(rootcipher);
        return -1;
    }
    if((err = gcry_cipher_decrypt(rootcipher, data->masterkey, 8, trackkey, 8)) != 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't calc key encryption key: %s"), gcry_strerror(err));
        gcry_cipher_close(rootcipher);
        return -1;
    }
    gcry_cipher_close(rootcipher);

    if(cached_cipher_init(&data->master, GCRY_CIPHER_MODE_ECB) != 0)
    {
        set_status_const(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't aquire DES ECB encryption"));
        return -1;
    }

    if(cached_cipher_init(&data->block, GCRY_CIPHER_MODE_CBC) != 0)
    {
        set_status_const(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't aquire DES CBC encryption"));
        cached_cipher_deinit(&data->master);
        return -1;
    }

    *dataptr = data;
    return 0;
}

int descrypt_decrypt(void * dataptr, unsigned char * block, size_t cryptlen,
                     const unsigned char * fragkey, struct himderrinfo * status)
{
    unsigned char finalfragkey[8];
    unsigned char mainkey[8];
    struct descrypt_data * data = dataptr;
    gcry_error_t err;

    xor_keys(finalfragkey, data->masterkey, fragkey);
    if((err = cached_cipher_prepare(&data->master, finalfragkey, NULL)) != 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't setup track key: %s"), gcry_strerror(err));
        return -1;
    }

    if((err = gcry_cipher_encrypt(data->master.cipher, mainkey, 8, block+16, 8)) != 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't calc block key: %s"), gcry_strerror(err));
        return -1;
    }

    if((err = cached_cipher_prepare(&data->block, mainkey, block + 24)) != 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't setup block key: %s"), gcry_strerror(err));
        return -1;
    }

    if((err = gcry_cipher_decrypt(data->block.cipher, block+32, cryptlen, block+32, cryptlen)) != 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't decrypt: %s"), gcry_strerror(err));
        return -1;
    }

    return 0;
}

void descrypt_close(void * dataptr)
{
    struct descrypt_data * data = dataptr;
    cached_cipher_deinit(&data->block);
    cached_cipher_deinit(&data->master);
    free(dataptr);
}

#endif
