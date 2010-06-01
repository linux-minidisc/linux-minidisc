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

struct descrypt_data {
    MCRYPT masterkeycipher;
    MCRYPT blockcipher;
    unsigned char lastkey[8];
    unsigned char blockcipher_inited;
};

int descrypt_open(void ** dataptr, const unsigned char * trackkey, 
                  unsigned int ekbnum, struct himderrinfo * status)
{
    static const unsigned char zerokey[] = {0,0,0,0,0,0,0,0};
    static const unsigned char masterkey[] = {0xf2,0x26,0x6c,0x64,0x64,0xc0,0xd6,0x5c};
    struct descrypt_data * data;
    int err;

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
    data->masterkeycipher = mcrypt_module_open("des", NULL, "ecb", NULL);
    if(!data->masterkeycipher)
    {
        set_status_const(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't aquire DES ECB encryption"));
        return -1;
    }
    if((err = mcrypt_generic_init(data->masterkeycipher, &masterkey, 8, NULL)) < 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't init DES: %s"), mcrypt_strerror(err));
        mcrypt_module_close(data->masterkeycipher);
        return -1;
    }
    data->blockcipher = mcrypt_module_open("des", NULL, "cbc", 0);
    if(!data->blockcipher)
    {
        set_status_const(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't aquire DES CBC encryption"));
        mcrypt_generic_deinit(data->masterkeycipher);
        mcrypt_module_close(data->masterkeycipher);
        return -1;
    }
    data->blockcipher_inited = 0;
    
    *dataptr = data;
    return 0;
}

int descrypt_decrypt(void * dataptr, unsigned char * block, size_t cryptlen, struct himderrinfo * status)
{
    unsigned char mainkey[8];
    struct descrypt_data * data = dataptr;
    int err;

    if(!data->blockcipher_inited || memcmp(data->lastkey, block+16, 8) != 0)
    {
        memcpy(mainkey, block+16, 8);
        if((err = mcrypt_generic(data->masterkeycipher, &mainkey, 8)) < 0)
        {
            set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't calc block key: %s"), mcrypt_strerror(err));
            return -1;
        }

        if(data->blockcipher_inited)
        {
            mcrypt_generic_deinit(data->blockcipher);
            data->blockcipher_inited = 0;
        }
        if((err = mcrypt_generic_init(data->blockcipher, mainkey, 8, block+24)) < 0)
        {
            set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't init CBC block cipher: %s"), mcrypt_strerror(err));
            return -1;
        }

        memcpy(data->lastkey, block+16, 8);
        data->blockcipher_inited = 1;
    }

    if((err = mdecrypt_generic(data->blockcipher, block+32, cryptlen)) < 0)
    {
        set_status_printf(status, HIMD_ERROR_ENCRYPTION_FAILURE, _("Can't decrypt: %s"), mcrypt_strerror(err));
        mcrypt_generic_deinit(data->blockcipher);
        return -1;
    }

    return 0;
}

void descrypt_close(void * dataptr)
{
    const struct descrypt_data * data = dataptr;
    if(data->blockcipher_inited)
        mcrypt_generic_deinit(data->blockcipher);
    mcrypt_module_close(data->blockcipher);
    mcrypt_generic_deinit(data->masterkeycipher);
    mcrypt_module_close(data->masterkeycipher);
    free(dataptr);
}

#endif
