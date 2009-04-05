#include <stdio.h>

/* Test vector: 1 CAEF24F1 -> A350796F */

int main(int argc, char ** argv)
{
    unsigned int trknum = 1;
    unsigned int discid;
    char dummy;
    
    if(argc != 3)
    {
        fprintf(stderr, "Please invoke as 'mp3key <tracknum> <discid>'\n"
                        "where tracknum is a decimal track number and discid is the last 8\n"
                        "digits (4 bytes) of the hex disc id\n");
        return 1;
    }
    
    if(sscanf(argv[1],"%d%c",&trknum,&dummy) != 1)
    {
        fprintf(stderr, "Track number is not a decimal integer\n");
        return 1;
    }
    if(sscanf(argv[2],"%x%c",&discid,&dummy) != 1)
    {
        fprintf(stderr, "Disk ID is not an 8 digit hex string\n");
        return 1;
    }
    
    unsigned int foo = ((trknum*0x6953B2ED)+0x6BAAB1) ^ discid;

    printf("%08X\n", foo & 0xFFFFFFFF);
    return 0;
}
