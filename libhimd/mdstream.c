#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>

#include "himd.h"
#include "himd_private.h"
#include "config.h"

#define _(x) (x)

int himd_blockstream_open(struct himd * himd, unsigned int firstfrag, struct himd_blockstream * stream, struct himderrinfo * status)
{
    struct fraginfo frag;
    unsigned int fragcount, fragnum, blockcount;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(firstfrag >= HIMD_FIRST_FRAGMENT, -1);
    g_return_val_if_fail(firstfrag <= HIMD_LAST_FRAGMENT, -1);
    g_return_val_if_fail(stream != NULL, -1);

    stream->himd = himd;

    for(fragcount = 0, blockcount = 0, fragnum = firstfrag;
        fragnum != 0; fragcount++)
    {
        if(himd_get_fragment_info(himd, fragnum, &frag, status) < 0)
            return -1;
        fragnum = frag.nextfrag;
        if(fragcount > HIMD_LAST_FRAGMENT)
        {
            set_status_printf(status, HIMD_ERROR_FRAGMENT_CHAIN_BROKEN,
                               _("Fragment chain starting at %d loops"), firstfrag);
            return -1;
        }
        blockcount += frag.lastblock - frag.firstblock + 1;
    }

    stream->frags = malloc(fragcount * sizeof stream->frags[0]);
    if(!stream->frags)
    {
        set_status_printf(status, HIMD_ERROR_OUT_OF_MEMORY,
                          _("Can't allocate %d fragments for chain starting at %d"), fragcount, firstfrag);
        return -1;
    }
    stream->fragcount = fragcount;
    stream->blockcount = blockcount;
    stream->curfragno = 0;

    for(fragcount = 0, fragnum = firstfrag; fragnum != 0; fragcount++)
    {
        if(himd_get_fragment_info(himd, fragnum, &stream->frags[fragcount], status) < 0)
            return -1;
        fragnum = stream->frags[fragcount].nextfrag;
    }

    stream->atdata = himd_open_file(himd, "atdata");
    if(!stream->atdata)
    {
        set_status_printf(status, HIMD_ERROR_CANT_OPEN_AUDIO,
                          _("Can't open audio data: %s"), g_strerror(errno));
        free(stream->frags);
        return -1;
    }

    stream->curblockno = stream->frags[0].firstblock;
    
    return 0;
}

void himd_blockstream_close(struct himd_blockstream * stream)
{
    fclose(stream->atdata);
    free(stream->frags);
}

int himd_blockstream_read(struct himd_blockstream * stream, unsigned char * block,
                            unsigned int * firstframe, unsigned int * lastframe, struct himderrinfo * status)
{
    g_return_val_if_fail(stream != NULL, -1);
    g_return_val_if_fail(block != NULL, -1);

    if(stream->curfragno == stream->fragcount)
    {
        set_status_const(status, HIMD_STATUS_AUDIO_EOF, _("EOF of audio stream reached"));
        return -1;
    }

    if(stream->curblockno == stream->frags[stream->curfragno].firstblock)
    {
        if(firstframe)
            *firstframe = stream->frags[stream->curfragno].firstframe;
        if(fseek(stream->atdata, stream->curblockno*16384L, SEEK_SET) < 0)
        {
            set_status_printf(status, HIMD_ERROR_CANT_SEEK_AUDIO,
                              _("Can't seek in audio data: %s"), g_strerror(errno));
            return -1;
        }
    }
    else if(firstframe)
        *firstframe = 0;

    if(fread(block, 16384, 1, stream->atdata) != 1)
    {
        if(feof(stream->atdata))
            set_status_printf(status, HIMD_ERROR_CANT_READ_AUDIO, _("Unexpected EOF while reading audio block %d"),stream->curblockno);
        else
            set_status_printf(status, HIMD_ERROR_CANT_READ_AUDIO, _("Read error on block audio %d: %s"), stream->curblockno, g_strerror(errno));
        return -1;
    }

    if(stream->curblockno == stream->frags[stream->curfragno].lastblock)
    {
        if(lastframe)
            *lastframe = stream->frags[stream->curfragno].lastframe - 1;
        stream->curfragno++;
        if(stream->curfragno < stream->fragcount)
            stream->curblockno = stream->frags[stream->curfragno].firstblock;
    }
    else
    {
        if(lastframe)
            *lastframe = beword16(block+4) - 1;
        stream->curblockno++;
    }
    return 0;
}

#ifdef HAVE_MAD

#include <mad.h>

int himd_mp3stream_open(struct himd * himd, unsigned int trackno, struct himd_mp3stream * stream, struct himderrinfo * status)
{
    struct trackinfo trkinfo;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(trackno >= HIMD_FIRST_TRACK, -1);
    g_return_val_if_fail(trackno <= HIMD_LAST_TRACK, -1);
    g_return_val_if_fail(stream != NULL, -1);

    if(himd_get_track_info(himd, trackno, &trkinfo, status) < 0)
        return -1;
    if(trkinfo.codec_id != CODEC_ATRAC3PLUS_OR_MPEG ||
       (trkinfo.codecinfo[0] & 3) != 3)
    {
        set_status_printf(status, HIMD_ERROR_BAD_AUDIO_CODEC,
                          _("Track %d does not contain MPEG data"), trackno);
        return -1;
    }

    if(himd_obtain_mp3key(himd, trackno, &stream->key, status) < 0)
        return -1;

    if(himd_blockstream_open(himd, trkinfo.firstfrag, &stream->stream, status) < 0)
        return -1;

    stream->frames = 0;
    stream->curframe = 0;
    stream->frameptrs = NULL;

    return 0;
}

int himd_mp3stream_read_frame(struct himd_mp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, struct himderrinfo * status)
{
    int gotdata = 1;

    g_return_val_if_fail(stream != NULL, -1);
    if(stream->curframe >= stream->frames)
    {
        unsigned int firstframe, lastframe;
        unsigned int i;
        unsigned int databytes;
        struct mad_stream madstream;
        struct mad_header madheader;

        /* Read block */
        if(himd_blockstream_read(&stream->stream, stream->blockbuf, &firstframe, &lastframe, status) < 0)
            return -1;

        if(firstframe > lastframe)
        {
            set_status_printf(status, HIMD_ERROR_BAD_FRAME_NUMBERS,
                       _("Last frame %u before first frame %u"),
                       lastframe, firstframe);
            return -1;
        }

        free(stream->frameptrs);
        stream->frameptrs = malloc((lastframe - firstframe + 2) * sizeof stream->frameptrs[0]);
        if(!stream->frameptrs)
        {
            set_status_printf(status, HIMD_ERROR_OUT_OF_MEMORY,
                       _("Can't allocate memory for %u frame pointers"),
                       lastframe-firstframe+2);
            return -1;
        }

        databytes = beword16(stream->blockbuf+8);
        /* Decrypt block */
        for(i = 0;i < (databytes & ~7U);i++)
            stream->blockbuf[i+0x20] ^= stream->key[i & 3];

        /* parse block */
        mad_stream_init(&madstream);
        mad_header_init(&madheader);

        mad_stream_buffer(&madstream, &stream->blockbuf[0x20],
                                      0x3fd0+MAD_BUFFER_GUARD);

        /* drop unneeded frames in front */
        while(firstframe > 0)
        {
            if(mad_header_decode(&madheader, &madstream) < 0)
            {
                set_status_printf(status, HIMD_ERROR_BAD_DATA_FORMAT,
                    _("Still %u frames to skip: %s"), firstframe, mad_stream_errorstr(&madstream));
                gotdata = 0;
                goto cleanup_decoder;
            }
            firstframe--;
            lastframe--;
        }
        

        /* store needed frames */
        for(i = 0;i <= lastframe;i++)
        {
            if(mad_header_decode(&madheader, &madstream) < 0 &&
                (madstream.error != MAD_ERROR_LOSTSYNC || i != lastframe-1))
            {
                set_status_printf(status, HIMD_ERROR_BAD_DATA_FORMAT,
                    _("Frame %u of %u to skip: %s"), i+1, lastframe, mad_stream_errorstr(&madstream));
                gotdata = 0;
                goto cleanup_decoder;
            }
            stream->frameptrs[i] = madstream.this_frame;
        }
        stream->frameptrs[i] = madstream.next_frame;
        stream->frames = lastframe+1;
        stream->curframe = 0;
cleanup_decoder:
        mad_header_finish(&madheader);
        mad_stream_finish(&madstream);
    }
    if(!gotdata)
        return -1;
    
    if(frameout)
        *frameout = stream->frameptrs[stream->curframe];
    if(lenout)
        *lenout = stream->frameptrs[stream->curframe + 1] - 
                  stream->frameptrs[stream->curframe];
    stream->curframe++;
    return 0;
}

void himd_mp3stream_close(struct himd_mp3stream * stream)
{
    free(stream->frameptrs);
    himd_blockstream_close(&stream->stream);
}

#else

int himd_mp3stream_open(struct himd * himd, unsigned int trackno, struct himd_mp3stream * stream)
{
    himd->status = HIMD_ERROR_DISABLED_FEATURE;
    g_strlcpy(himd->statusmsg, _("Can't open mp3 track: Compiled without mad library"), sizeof himd->statusmsg);
    return -1;
}

int himd_mp3stream_read_frame(struct himd_mp3stream * stream, const unsigned char ** frameout, int * lenout)
{
    stream->stream.status = HIMD_ERROR_DISABLED_FEATURE;
    g_strlcpy(stream->stream.statusmsg, _("Can't do mp3 read: Compiled without mad library"), sizeof stream->stream.statusmsg);
    return -1;
}

void himd_mp3stream_close(struct himd_mp3stream * stream)
{
}
#endif

#ifdef LIBMCRYPT24
#include <string.h>

int himd_pcmstream_open(struct himd * himd, unsigned int trackno, struct himd_pcmstream * stream, struct himderrinfo * status)
{
    struct trackinfo trkinfo;
    static const unsigned char zerokey[] = {0,0,0,0,0,0,0,0};

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(trackno >= HIMD_FIRST_TRACK, -1);
    g_return_val_if_fail(trackno <= HIMD_LAST_TRACK, -1);
    g_return_val_if_fail(stream != NULL, -1);

    if(himd_get_track_info(himd, trackno, &trkinfo, status) < 0)
        return -1;
    if(trkinfo.codec_id != CODEC_LPCM)
    {
        set_status_printf(status, HIMD_ERROR_BAD_AUDIO_CODEC,
                          _("Track %d does not contain PCM data"), trackno);
        return -1;
    }
    if(memcmp(trkinfo.key, zerokey, 8) != 0)
    {
        set_status_printf(status, HIMD_ERROR_UNSUPPORTED_ENCRYPTION,
                          _("Track %d uses strong encryption"), trackno);
        return -1;
    }
    if(himd_blockstream_open(himd, trkinfo.firstfrag, &stream->stream, status) < 0)
        return -1;

    if(descrypt_open(&stream->cryptinfo, status) < 0)
    {
        himd_blockstream_close(&stream->stream);
        return -1;
    }
    return 0;
}

int himd_pcmstream_read_frame(struct himd_pcmstream * stream, const unsigned char ** frameout, unsigned int * lenout, struct himderrinfo * status)
{
    if(himd_blockstream_read(&stream->stream, stream->blockbuf, NULL, NULL, status) < 0)
        return -1;
    if(descrypt_decrypt(stream->cryptinfo, stream->blockbuf, 0x3fc0, status) < 0)
        return -1;
    if(frameout)
        *frameout = stream->blockbuf+32;
    if(lenout)
        *lenout = 0x3fc0;
    return 0;
}

void himd_pcmstream_close(struct himd_pcmstream * stream)
{
    himd_blockstream_close(&stream->stream);
    descrypt_close(stream->cryptinfo);
}

#else

int himd_pcmstream_open(struct himd * himd, unsigned int trackno, struct himd_pcmstream * stream, struct himderrinfo * status)
{
    himd->status = HIMD_ERROR_DISABLED_FEATURE;
    g_strlcpy(himd->statusmsg, _("Can't open pcm track: Compiled without mcrypt library"), sizeof himd->statusmsg)
    return -1;
}

int himd_pcmstream_read_frame(struct himd_pcmstream * stream, const unsigned char ** frameout, unsigned int * lenout, struct himderrinfo * status);
{
    stream->stream.status = HIMD_ERROR_DISABLED_FEATURE;
    g_strlcpy(stream->stream.statusmsg, _("Can't do pcm read: Compiled without mcrypt library"), sizeof stream->stream.statusmsg);
    return -1;
}

void himd_pcmstream_close(struct himd_pcmstream * stream)
{
}

#endif
