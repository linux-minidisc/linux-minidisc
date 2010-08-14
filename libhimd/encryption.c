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

#ifdef CONFIG_WITH_MCRYPT
#include "mcrypt.h"
#include "himd_private.h"
#include <string.h>

struct cached_cipher {
    unsigned char key[8];
    MCRYPT cipher;
    int valid;
};

struct descrypt_data {
    struct cached_cipher master;
    struct cached_cipher block;
    unsigned char masterkey[8];
};

static int cached_cipher_init(struct cached_cipher * cipher, char * destype)
{
    cipher->cipher = mcrypt_module_open("des", NULL, destype, NULL);
    if(!cipher->cipher)
        return -1;

    cipher->valid = 0;
    return 0;
}

/* iv should be NULL for ECB mode */
static int cached_cipher_prepare(struct cached_cipher * cipher,
                                 unsigned char * key, unsigned char * iv)
{
    int err;

    /* not yet initialized or new key */
    if(!cipher->valid ||
       memcmp(cipher->key, key, 8))
    {
        if(cipher->valid)
            mcrypt_generic_deinit(cipher->cipher);
        cipher->valid = 0;

        err = mcrypt_generic_init(cipher->cipher, key, 8, iv);
        if(err < 0)
            return err;

        memcpy(cipher->key, key, 8);
        cipher->valid = 1;
    }
    else if(iv)		/* update IV, works for CBC decryption */
    {
        unsigned char dummy[8];
        memcpy(dummy, iv, 8);
        err = mdecrypt_generic(cipher->cipher, dummy, 8);
        if(err < 0)
            return err;
    }
    return 0;
}

static void cached_cipher_deinit(struct cached_cipher * cipher)
{
    if(cipher->valid)
        mcrypt_generic_deinit(cipher->cipher);
    mcrypt_module_close(cipher->cipher);
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
    static const unsigned char zerokey[] = {0,0,0,0,0,0,0,0};
    static const unsigned char masterkey[] = {0xf2,0x26,0x6c,0x64,0x64,0xc0,0xd6,0x5c};
    struct descrypt_data * data;

    if(ekbnum != 0x00010012)
    {
        set_status_const(status, HIMD_ERROR_UNSUPPORTED_ENCRYPTION, _("EKB %08x unsupported"));
        return -1;
    }

    if(memcmp(trackkey, zerokey, 8) != 0)
    {
        set_status_const(status, HIMD_ERROR_UNSUPPORTED_ENCRYPTION,
                          _("Track uses strong encryption"));
        return -1;
    }

    data = malloc(sizeof *data);
    if(!data)
    {
        set_status_const(status, HIMD_ERROR_OUT_OF_MEMORY, _("Can't allocate crypt helper structure"));
        return -1;
    }
    if(cached_cipher_init(&data->master, "ecb") < 0)
    {
        set_status_const(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't aquire DES ECB encryption"));
        return -1;
    }

    memcpy(data->masterkey, masterkey, 8);
    if(cached_cipher_init(&data->block, "cbc") < 0)
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
    int err;

    xor_keys(finalfragkey, data->masterkey, fragkey);
    if((err = cached_cipher_prepare(&data->master, finalfragkey, NULL)) < 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't setup track key: %s"), mcrypt_strerror(err));
        return -1;
    }

    memcpy(mainkey, block+16, 8);
    if((err = mcrypt_generic(data->master.cipher, mainkey, 8)) < 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't calc block key: %s"), mcrypt_strerror(err));
        return -1;
    }

    if((err = cached_cipher_prepare(&data->block, mainkey, block + 24)) < 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't setup block key: %s"), mcrypt_strerror(err));
        return -1;
    }

    if((err = mdecrypt_generic(data->block.cipher, block+32, cryptlen)) < 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't decrypt: %s"), mcrypt_strerror(err));
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
