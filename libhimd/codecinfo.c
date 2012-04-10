#include "codecinfo.h"
#include <glib.h>
#include <glib/gprintf.h>

#define MPEG_I_SAMPLES_PER_FRAME 384
#define MPEG_II_III_SAMPLES_PER_FRAME (3*MPEG_I_SAMPLES_PER_FRAME)

/* for MPEG, frame size may slightly vary (+4 for layer I, +1 for layer II/III) */
unsigned int sony_codecinfo_bytesperframe(const struct sony_codecinfo *ci)
{
    g_return_val_if_fail(ci != NULL, 0);

    if(sony_codecinfo_is_lpcm(ci))
        return SONY_VIRTUAL_LPCM_FRAMESIZE;
    if(sony_codecinfo_is_at3(ci))
        return ci->codecinfo[2] * 8;
    if(sony_codecinfo_is_at3p(ci))
        return ((((ci->codecinfo[1] << 8) | ci->codecinfo[2]) & 0x3ff) + 1) * 8;
    if(sony_codecinfo_is_mpeg(ci)) {
        unsigned int mask = ~0;
        if((ci->codecinfo[3] & 0xC0) == 0xC0)
            mask = ~3;	/* MPEG Layer I is specified to work DWORDs, not on
                           bytes */
        return (sony_codecinfo_samplesperframe(ci) *	/* samples / frame */
                (sony_codecinfo_kbps(ci)*125) /		/* bytes / second */
                 sony_codecinfo_samplerate(ci))		/* samples / second */
               & mask;
            /* samples and seconds cancel, bytes / frame remains.
               Truncation is indeed as required. */
    }
    return 0;
}

unsigned int sony_codecinfo_samplesperframe(const struct sony_codecinfo *ci)
{
    g_return_val_if_fail(ci != NULL, 0);

    if(sony_codecinfo_is_lpcm(ci))
        return SONY_VIRTUAL_LPCM_FRAMESIZE / 4;
    if(sony_codecinfo_is_at3(ci))
        return SONY_ATRAC3_SAMPLES_PER_FRAME;
    if(sony_codecinfo_is_at3p(ci))
        return SONY_ATRAC3P_SAMPLES_PER_FRAME;
    if(sony_codecinfo_is_mpeg(ci))
    {
        if((ci->codecinfo[3] & 0x30) == 0x30)	/* MPEG Layer I */
            return MPEG_I_SAMPLES_PER_FRAME;
        else					/* MPEG Layer II & III (MP3) */
            return MPEG_II_III_SAMPLES_PER_FRAME;
    }
    return 0;
}

static const long int atracrates[8] = {32000,44100,48000,88200,96000};
static const long int mpegrates[4] = {44100,48000,32000};

unsigned long sony_codecinfo_samplerate(const struct sony_codecinfo *ci)
{
    g_return_val_if_fail(ci != NULL, 0);

    if(sony_codecinfo_is_lpcm(ci))
        return 44100;
    if(sony_codecinfo_is_at3(ci) || sony_codecinfo_is_at3p(ci))
        return atracrates[ci->codecinfo[1] >> 5];
    if(sony_codecinfo_is_mpeg(ci))
        return mpegrates[ci->codecinfo[4] >> 6] / (4 - (ci->codecinfo[3] >> 6));
    return 0;
}

unsigned int sony_codecinfo_kbps(const struct sony_codecinfo *ci)
{
    g_return_val_if_fail(ci != NULL, 0);

    if(!sony_codecinfo_is_mpeg(ci))
        /* ATRAC & LPCM: calculate kbps from frame size */
        return (unsigned long)sony_codecinfo_bytesperframe(ci) *
               sony_codecinfo_samplerate(ci) /
               (125*sony_codecinfo_samplesperframe(ci));
                /* 125 = bytes per kbit */
    else {
        /* MPEG: kbps is well-defined, bytes/frame is calculated from it */
        static const short v1l1[16] = {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0};
        static const short v1l2[16] = {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384,0};
        static const short v1l3[16] = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
        static const short v2l1[16] = {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0};
        static const short v2l23[16] = {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0};
        static const short* vl[2][3] = {{v1l1,v1l2,v1l3},{v2l1,v2l23,v2l23}};
        if((ci->codecinfo[3] & 0xC0) == 0x40 || (ci->codecinfo[3] & 0x30) == 0)
            return 0;	/* invalid header info */
        return vl[1-((ci->codecinfo[3] & 0x40) >> 6)]
                 [3-((ci->codecinfo[3] & 0x30) >> 4)]
                 [ci->codecinfo[3] & 0xF];
    }
}

unsigned int sony_codecinfo_seconds(const struct sony_codecinfo *ci, unsigned int frames)
{
    g_return_val_if_fail(ci != NULL, 0);
    return ((guint64)frames * sony_codecinfo_samplesperframe(ci)) / 
             sony_codecinfo_samplerate(ci);
}

const char * sony_codecinfo_codecname(const struct sony_codecinfo *ci)
{
    static char buffer[5];

    g_return_val_if_fail(ci != NULL, "(nullptr)");

    if(sony_codecinfo_is_lpcm(ci))
        return "LPCM";
    if(sony_codecinfo_is_at3(ci))
        return "AT3 ";
    if(sony_codecinfo_is_at3p(ci))
        return "AT3+";
    if(sony_codecinfo_is_mpeg(ci))
        return "MPEG";
    /* codec_id is supposed to be an unsigned char. Adding the mask
       anyway to prevent a buffer overflow if someone cooses to enlarge
       it or compile on a machine with CHAR_BITS > 13 */
    sprintf(buffer,"%4d",ci->codec_id & 0xFF);
    return buffer;
}
