#include <stdio.h>
#include <string.h>
#include <mad.h>

#define ALL_FRAMES 16384
int decodemp3(const unsigned char * key, const char * block, int skipframes, int maxframes,
                char * decoded, int * len)
{
    struct mad_stream stream;
    struct mad_header header;
    int framecount = block[4]*256+block[5];
    char xorred_frame[16304+MAD_BUFFER_GUARD];
    int i;
    int result = 0;

    for(i = 0;i < 16304;i++)
        xorred_frame[i] = block[i+32] ^ key[i&3];
    memset(xorred_frame+16304,0,MAD_BUFFER_GUARD);

    mad_stream_init(&stream);
    mad_header_init(&header);
    
    mad_stream_buffer(&stream, xorred_frame, 16304+MAD_BUFFER_GUARD);

    if(framecount > maxframes)
        framecount = maxframes;
    
    *len = 0;
    for(i = 0; i < framecount; i++)
    {
        if(mad_header_decode(&header, &stream) < 0
           && (stream.error != MAD_ERROR_LOSTSYNC ||
               i != framecount-1))
        {
            printf("%d\n",stream.error);
            result = -1;
            break;
        }
        if(skipframes)
            skipframes--;
        else
        {
            memcpy(decoded + *len,stream.this_frame,stream.next_frame - stream.this_frame);
            *len += stream.next_frame - stream.this_frame;
            result++;
        }
    }
    
    mad_header_finish(&header);
    mad_stream_finish(&stream);
    return result;
}

int main(void)
{
    FILE * f = fopen("/media/disk/HMDHIFI/ATDATA02.HMA","rb");
    const unsigned char key[] =  {0x00,0x69,0xF7,0x7A};
// 00641418
    int firstblock = 0x60;
    int firstframe = 0x0;
    int lastblock = 0x12d;
    int lastframe = 0x1e;
    int b;
    int len;
    char block[16384];
    char decoded[16384];
    
    fseek(f,firstblock*16384L,SEEK_SET);
    for(b = firstblock;b <= lastblock;++b)
    {
        fread(block,1,16384,f);
        if(decodemp3(key,block,(b == firstblock) ? firstframe : 0,
                               (b == lastblock) ? lastframe : ALL_FRAMES,
                     decoded, &len) < 0)
        {
            fprintf(stderr,"decoder failed at block %d\n",b);
            return 1;
        }
        fwrite(decoded,1,len,stdout);
    }
    return 0;
}
