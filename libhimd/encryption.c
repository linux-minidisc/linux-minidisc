#include "himd.h"

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
