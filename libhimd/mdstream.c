#include <stdlib.h>
#include <errno.h>
#include <glib.h>

#include "himd.h"
#include "himd_private.h"

#define _(x) (x)

int himd_blockstream_open(struct himd * himd, unsigned int firstfrag, struct himd_blockstream * stream)
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
        if(himd_get_fragment_info(himd, fragnum, &frag) < 0)
            return -1;
        fragnum = frag.nextfrag;
        if(fragcount > HIMD_LAST_FRAGMENT)
        {
            himd->status = HIMD_ERROR_FRAGMENT_CHAIN_BROKEN;
            g_snprintf(himd->statusmsg, sizeof himd->statusmsg, _("Fragment chain starting at %d loops"), firstfrag);
            return -1;
        }
        blockcount += frag.lastblock - frag.firstblock + 1;
    }

    stream->frags = malloc(fragcount * sizeof stream->frags[0]);
    if(!stream->frags)
    {
        himd->status = HIMD_ERROR_OUT_OF_MEMORY;
        g_snprintf(himd->statusmsg, sizeof himd->statusmsg, _("Can't allocate %d fragments for chain starting at %d"), fragcount, firstfrag);
        return -1;
    }
    stream->fragcount = fragcount;
    stream->blockcount = blockcount;
    stream->curfragno = 0;

    for(fragcount = 0, fragnum = firstfrag; fragnum != 0; fragcount++)
    {
        if(himd_get_fragment_info(himd, fragnum, &stream->frags[fragcount]) < 0)
            return -1;
        fragnum = stream->frags[fragcount].nextfrag;
    }

    stream->atdata = himd_open_file(himd, "atdata");
    if(!stream->atdata)
    {
        himd->status = HIMD_ERROR_CANT_OPEN_AUDIO;
        g_snprintf(himd->statusmsg, sizeof himd->statusmsg, _("Can't open audio data: %s"), g_strerror(errno));
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
                            int * firstframe, int * lastframe)
{
    g_return_val_if_fail(stream != NULL, -1);
    g_return_val_if_fail(block != NULL, -1);

    if(stream->curfragno == stream->fragcount)
    {
        stream->status = HIMD_STATUS_AUDIO_EOF;
        g_strlcpy(stream->statusmsg, _("EOF of audio stream reached"), sizeof stream->statusmsg);
        return -1;
    }

    if(stream->curblockno == stream->frags[stream->curfragno].firstblock)
    {
        if(firstframe)
            *firstframe = stream->frags[stream->curfragno].firstframe;
        if(fseek(stream->atdata, stream->curblockno*16384L, SEEK_SET) < 0)
        {
            stream->status = HIMD_ERROR_CANT_SEEK_AUDIO;
            g_snprintf(stream->statusmsg, sizeof stream->statusmsg, _("Can't seek in audio data: %s"), g_strerror(errno));
            return -1;
        }
    }
    else if(firstframe)
        *firstframe = 0;

    if(fread(block, 16384, 1, stream->atdata) != 1)
    {
        stream->status = HIMD_ERROR_CANT_READ_AUDIO;
        if(feof(stream->atdata))
            g_snprintf(stream->statusmsg, sizeof stream->statusmsg, _("Unexpected EOF while reading audio block %d"),stream->curblockno);
        else
            g_snprintf(stream->statusmsg, sizeof stream->statusmsg, _("Read error on block audio %d: %s"), stream->curblockno, g_strerror(errno));
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

