#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>

#include "himd.h"
#include "himd_private.h"

#define _(x) (x)

int himd_blockstream_open(struct himd * himd, unsigned int firstfrag, unsigned int frags_per_block, struct himd_blockstream * stream, struct himderrinfo * status)
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

    stream->atdata = himd_open_file(himd, "ATDATA");
    if(!stream->atdata)
    {
        set_status_printf(status, HIMD_ERROR_CANT_OPEN_AUDIO,
                          _("Can't open audio data: %s"), g_strerror(errno));
        free(stream->frags);
        return -1;
    }

    stream->curblockno = stream->frags[0].firstblock;
    stream->frames_per_block = frags_per_block;
    
    return 0;
}

void himd_blockstream_close(struct himd_blockstream * stream)
{
    fclose(stream->atdata);
    free(stream->frags);
}

static inline int is_mpeg(struct himd_blockstream * stream)
{
    return stream->frames_per_block == TRACK_IS_MPEG;
}

int himd_blockstream_read(struct himd_blockstream * stream, unsigned char * block,
                            unsigned int * firstframe, unsigned int * lastframe,
                            unsigned char * fragkey, struct himderrinfo * status)
{
    struct fraginfo * curfrag;

    g_return_val_if_fail(stream != NULL, -1);
    g_return_val_if_fail(block != NULL, -1);

    if(stream->curfragno == stream->fragcount)
    {
        set_status_const(status, HIMD_STATUS_AUDIO_EOF, _("EOF of audio stream reached"));
        return -1;
    }

    curfrag = &stream->frags[stream->curfragno];

    if(stream->curblockno == curfrag->firstblock)
    {
        if(firstframe)
            *firstframe = curfrag->firstframe;
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

    if(fragkey)
        memcpy(fragkey, curfrag->key, sizeof curfrag->key);

    if(stream->curblockno == curfrag->lastblock)
    {
        if(lastframe)
        {
            if(is_mpeg(stream))
                *lastframe = curfrag->lastframe - 1;
            else
                *lastframe = curfrag->lastframe;
        }
        stream->curfragno++;
        curfrag++;
        if(stream->curfragno < stream->fragcount)
            stream->curblockno = curfrag->firstblock;
    }
    else
    {
        if(lastframe)
        {
            if(is_mpeg(stream))
                *lastframe = beword16(block+4) - 1;
            else
                *lastframe = stream->frames_per_block - 1;
        }
        stream->curblockno++;
    }

    return 0;
}

int himd_writestream_open(struct himd * himd, struct himd_writestream * stream,
		       unsigned int * out_first_blockno, unsigned int * out_last_blockno, struct himderrinfo * status)
{
    struct himd_holelist hole_list;
    int firstblock, lastblock=0;
    int block_offset=0;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(stream != NULL, -1);
    g_return_val_if_fail(status != NULL, -1);

    stream->himd = himd;
    stream->atdata = himd_open_file(himd, "ATDATA");
    if(!stream->atdata)
	{
	    fprintf(stderr, "Invalid filehandle %d", stream->atdata);
	    perror("DBG: cannot open ATDATA file for writing\n");
	    return -1;
	}

    // himd_find_holes
    if( himd_find_holes(stream->himd, &hole_list, status) < 0)
	{
	    puts(status->statusmsg);
	    exit(1);
	}

    // get pointer to freespace
    firstblock   =  hole_list.holes[0].firstblock;
    lastblock    =  hole_list.holes[0].lastblock;
    block_offset = firstblock * HIMD_BLOCKINFO_SIZE;

    // set position where to start writing blocks
    if(fseek(stream->atdata, block_offset, SEEK_SET) != 0)
	{
	    fprintf(stderr, "Error fseeking atdata\n");
	}
    stream->curblockno = firstblock;

    if( (out_first_blockno != NULL) && (out_last_blockno != NULL) )
	{
	    *out_first_blockno = firstblock;
	    *out_last_blockno = lastblock;
	}

    return 0;
}

void himd_writestream_close(struct himd_writestream * stream)
{
    fclose(stream->atdata);
}

static void setblock(struct blockinfo * b, unsigned char * blockbuffer)
{
    memset(blockbuffer, 0, HIMD_BLOCKINFO_SIZE);
    strncpy((char*)blockbuffer, "SPMA", sizeof(unsigned int));
    setbeword16(blockbuffer+4, b->nframes);
    setbeword16(blockbuffer+6, b->mcode);
    setbeword16(blockbuffer+8, b->lendata);
    setbeword32(blockbuffer+12, b->serial_number);
    memcpy(blockbuffer+16, &b->key, 8);
    memcpy(blockbuffer+24, &b->iv, 8);
    memcpy(blockbuffer+32, &b->audio_data, HIMD_AUDIO_SIZE);
    setbeword16(blockbuffer+16374, b->mcode);
    setbeword32(blockbuffer+16380, b->serial_number);
}

int himd_writestream_write(struct himd_writestream * stream, struct blockinfo * audioblock, struct himderrinfo *status)
{
    unsigned char data[HIMD_BLOCKINFO_SIZE];
    g_return_val_if_fail(stream != NULL, -1);
    g_return_val_if_fail(audioblock != NULL, -1);
    status = status;
    stream = stream;

    // serialize the block descriptor
    setblock(audioblock, data);

    // write the block descriptor to the current position in the stream at 'stream->curblockno'
    if(fwrite(data, 16384, 1, stream->atdata) != 1)
	{
	    perror("fwrite block\n");
	    fprintf(stderr, "Error writing block to position %d\n", stream->curblockno);
	    return -1;
	}
    return 0;
}

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

    if(himd_blockstream_open(himd, trkinfo.firstfrag, TRACK_IS_MPEG, &stream->stream, status) < 0)
        return -1;

    stream->frames = 0;
    stream->curframe = 0;
    stream->frameptrs = NULL;

    return 0;
}

#ifdef CONFIG_WITH_MAD
#include <mad.h>

static int himd_mp3stream_split_frames(struct himd_mp3stream * stream, unsigned int databytes, unsigned int firstframe, unsigned int lastframe, struct himderrinfo * status)
{
    int gotdata = 1;
    unsigned int i;
    struct mad_stream madstream;
    struct mad_header madheader;

    /* stream->frameptrs is NULL if the current frame has not been splitted yet */
    g_warn_if_fail(stream->frameptrs == NULL);

    stream->frameptrs = malloc((lastframe - firstframe + 2) * sizeof stream->frameptrs[0]);
    if(!stream->frameptrs)
    {
        set_status_printf(status, HIMD_ERROR_OUT_OF_MEMORY,
                   _("Can't allocate memory for %u frame pointers"),
                   lastframe-firstframe+2);
        return -1;
    }
    /* parse block */
    mad_stream_init(&madstream);
    mad_header_init(&madheader);

    mad_stream_buffer(&madstream, &stream->blockbuf[0x20],
                                  databytes+MAD_BUFFER_GUARD);

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
            (madstream.error != MAD_ERROR_LOSTSYNC || i != lastframe))
        {
            set_status_printf(status, HIMD_ERROR_BAD_DATA_FORMAT,
                _("Frame %u of %u to store: %s"), i+1, lastframe, mad_stream_errorstr(&madstream));
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

    if(!gotdata)
        return -1;

    return 0;
}

#endif

int himd_mp3stream_read_block(struct himd_mp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, unsigned int * framecount, struct himderrinfo * status)
{
    unsigned int i;
    unsigned int firstframe, lastframe;
    unsigned int dataframes, databytes;

    /* partial block remaining, return all remaining frames */
    if(stream->curframe < stream->frames)
    {
        if(frameout)
            *frameout = stream->frameptrs[stream->curframe];
        if(lenout)
            *lenout = stream->frameptrs[stream->frames] - 
                      stream->frameptrs[stream->curframe];
        if(framecount)
            *framecount = stream->frames - stream->curframe;

        stream->curframe = stream->frames;
        return 0;
    }
    
    /* need to read next block */
    if(himd_blockstream_read(&stream->stream, stream->blockbuf,
                             &firstframe, &lastframe, NULL, status) < 0)
        return -1;

    free(stream->frameptrs);
    stream->frameptrs = NULL;

    if(firstframe > lastframe)
    {
        set_status_printf(status, HIMD_ERROR_BAD_FRAME_NUMBERS,
                   _("Last frame %u before first frame %u"),
                   lastframe, firstframe);
        return -1;
    }

    dataframes = beword16(stream->blockbuf+4);
    databytes = beword16(stream->blockbuf+8);

    if(databytes > 0x3FC0)
    {
        set_status_printf(status, HIMD_ERROR_BAD_DATA_FORMAT,
                   _("Block contains %u MPEG data bytes, which is too much"),
                   databytes);
        return -1;
    }

    if(lastframe >= dataframes)
    {
        set_status_printf(status, HIMD_ERROR_BAD_FRAME_NUMBERS,
                   _("Last requested frame %u past number of frames %u"),
                   lastframe, dataframes);
        return -1;
    }

    /* Decrypt block */
    for(i = 0;i < (databytes & ~7U);i++)
        stream->blockbuf[i+0x20] ^= stream->key[i & 3];

    /* Indicate completely consumed block 
       be sure to set this *before* writing to *framecont,
       it might alias stream->frames! */
    stream->frames = 0;
    stream->curframe = 0;

    /* The common case - all frames belong to the stream to read.
       If compiled without MAD, always put all frames into the block */
#ifndef CONFIG_WITH_MAD
    if(firstframe == 0 && lastframe == dataframes - 1)
#endif
    {
        if(frameout)
            *frameout = stream->blockbuf + 0x20;

        if(lenout)
            *lenout = databytes;

        if(framecount)
            *framecount = dataframes;

        return 0;
    }

#ifdef CONFIG_WITH_MAD
    if(himd_mp3stream_split_frames(stream, databytes, firstframe, lastframe, status) < 0)
        return -1;

    if(*framecount)
        *framecount = lastframe - firstframe + 1;
#endif
    return 0;
}

#ifdef CONFIG_WITH_MAD
int himd_mp3stream_read_frame(struct himd_mp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, struct himderrinfo * status)
{
    g_return_val_if_fail(stream != NULL, -1);
    if(stream->curframe >= stream->frames)
    {
        unsigned int databytes, framecount;
        if(himd_mp3stream_read_block(stream, NULL, &databytes, &framecount, status) < 0)
            return -1;
        /* if whole block should be used, it is not yet splitted */
        if(!stream->frameptrs &&
            himd_mp3stream_split_frames(stream, databytes, 0, framecount, status) < 0)
            return -1;
    }
    
    if(frameout)
        *frameout = stream->frameptrs[stream->curframe];
    if(lenout)
        *lenout = stream->frameptrs[stream->curframe + 1] - 
                  stream->frameptrs[stream->curframe];
    stream->curframe++;
    return 0;
}

#else

int himd_mp3stream_read_frame(struct himd_mp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, struct himderrinfo * status)
{
    (void)stream;
    (void)frameout;
    (void)lenout;
    (void)status;
    set_status_const(status, HIMD_ERROR_DISABLED_FEATURE, _("Can't do mp3 framewise read: Compiled without mad library"));
    return -1;
}

#endif

void himd_mp3stream_close(struct himd_mp3stream * stream)
{
    g_return_if_fail(stream != NULL);
    free(stream->frameptrs);
    himd_blockstream_close(&stream->stream);
}

#ifdef CONFIG_WITH_MCRYPT
#include <string.h>

int himd_nonmp3stream_open(struct himd * himd, unsigned int trackno, struct himd_nonmp3stream * stream, struct himderrinfo * status)
{
    struct trackinfo trkinfo;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(trackno >= HIMD_FIRST_TRACK, -1);
    g_return_val_if_fail(trackno <= HIMD_LAST_TRACK, -1);
    g_return_val_if_fail(stream != NULL, -1);

    if(himd_get_track_info(himd, trackno, &trkinfo, status) < 0)
        return -1;
    if((trkinfo.codec_id != CODEC_LPCM) &&
       (trkinfo.codec_id != CODEC_ATRAC3) &&
       (trkinfo.codec_id != CODEC_ATRAC3PLUS_OR_MPEG ||
                           (trkinfo.codecinfo[0] & 3) != 0))
    {
        set_status_printf(status, HIMD_ERROR_BAD_AUDIO_CODEC,
                          _("Track %d does not contain PCM, ATRAC3 or ATRAC3+ data"), trackno);
        return -1;
    }
    if(himd_blockstream_open(himd, trkinfo.firstfrag, himd_trackinfo_framesperblock(&trkinfo), &stream->stream, status) < 0)
        return -1;

    if(descrypt_open(&stream->cryptinfo, trkinfo.key, trkinfo.ekbnum, status) < 0)
    {
        himd_blockstream_close(&stream->stream);
        return -1;
    }
    stream->framesize = himd_trackinfo_framesize(&trkinfo);
    stream->framesleft = 0;
    return 0;
}

int himd_nonmp3stream_read_frame(struct himd_nonmp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, struct himderrinfo * status)
{
    g_return_val_if_fail(stream != NULL, -1);

    if(!stream->framesleft)
        if(himd_nonmp3stream_read_block(stream, &stream->frameptr, NULL, &stream->framesleft, status) < 0)
            return -1;

    if(frameout)
        *frameout = (unsigned char *)stream->frameptr;
    if(lenout)
        *lenout = stream->framesize;

    stream->framesleft--;
    stream->frameptr += stream->framesize;
    return 0;
}

int himd_nonmp3stream_read_block(struct himd_nonmp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, unsigned int * framecount, struct himderrinfo * status)
{
    unsigned int firstframe, lastframe;
    unsigned char fragkey[8];

    g_return_val_if_fail(stream != NULL, -1);
    /* if partial block left */
    if(stream->framesleft)
    {
        if(frameout)
            *frameout = stream->frameptr;
        if(lenout)
            *lenout = stream->framesleft * stream->framesize;
        if(framecount)
            *framecount = stream->framesleft;

        stream->framesleft = 0;
        return 0;
    }
    
    if(himd_blockstream_read(&stream->stream, stream->blockbuf,
                             &firstframe, &lastframe, fragkey, status) < 0)
        return -1;
    if(descrypt_decrypt(stream->cryptinfo, stream->blockbuf,
                        stream->framesize * stream->stream.frames_per_block,
                        fragkey, status) < 0)
        return -1;
    if(frameout)
        *frameout = stream->blockbuf+32 + firstframe * stream->framesize;
    if(lenout)
        *lenout = stream->framesize * ((lastframe-firstframe)+1);
    if(framecount)
        *framecount = lastframe - firstframe + 1;
    stream->framesleft = 0;
    return 0;
}

void himd_nonmp3stream_close(struct himd_nonmp3stream * stream)
{
    g_return_if_fail(stream != NULL);

    himd_blockstream_close(&stream->stream);
    descrypt_close(stream->cryptinfo);
}

#else

int himd_nonmp3stream_open(struct himd * himd, unsigned int trackno, struct himd_nonmp3stream * stream, struct himderrinfo * status)
{
    set_status_const(status, HIMD_ERROR_DISABLED_FEATURE, _("Can't open non-mp3 track: Compiled without mcrypt library"));
    return -1;
}

int himd_nonmp3stream_read_frame(struct himd_nonmp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, struct himderrinfo * status)
{
    set_status_const(status, HIMD_ERROR_DISABLED_FEATURE, _("Can't do non-mp3 read: Compiled without mcrypt library"));
    return -1;
}

void himd_nonmp3stream_close(struct himd_nonmp3stream * stream)
{
}

#endif
