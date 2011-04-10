#include <string.h>
#include <glib.h>
#include "himd.h"

#include "himd_private.h"

#define _(x) (x)

static unsigned char * get_track(struct himd * himd, unsigned int idx)
{
    return himd->tifdata +  0x8000 + 0x50 * idx;
}

static unsigned char * get_frag(struct himd * himd, unsigned int idx)
{
    return himd->tifdata + 0x30000 + 0x10 * idx;
}

static unsigned char * get_strchunk(struct himd * himd, unsigned int idx)
{
    return himd->tifdata + 0x40000 + 0x10 * idx;
}

static int strtype(unsigned char * stringchunk)
{
    return stringchunk[14] >> 4;
}

static int strlink(unsigned char * stringchunk)
{
    return beword16(stringchunk+14) & 0xFFF;
}

unsigned int himd_track_count(struct himd * himd)
{
    return beword16(himd->tifdata + 0x100);
}

unsigned int himd_get_trackslot(struct himd * himd, unsigned int idx, struct himderrinfo * status)
{
    if(idx >= himd_track_count(himd))
    {
        set_status_printf(status, HIMD_ERROR_NO_SUCH_TRACK, _("Track %d of %d requested"));
        return 0;
    }
    return beword16(himd->tifdata + 0x102 + 2*idx);
}

static void get_dostime(struct tm * tm, unsigned const char * bytes)
{
    unsigned int thetime = beword16(bytes+2);
    unsigned int thedate = beword16(bytes);
    tm->tm_sec = (thetime & 0x1F)*2;
    tm->tm_min = (thetime & 0x7E0) >> 5;
    tm->tm_hour = (thetime & 0xF100) >> 11;
    tm->tm_mday = (thedate & 0x1F);
    tm->tm_mon = ((thedate & 0x1E0) >> 5) - 1;
    tm->tm_year = ((thedate & 0xFE00) >> 9) + 80;
}


static void dos_settime(unsigned char * buffer, const struct tm * tm)
{
   setbeword16(buffer, (tm->tm_mday) |
                       ((tm->tm_mon + 1) << 5) |
                       ((tm->tm_year - 80) << 9));
   setbeword16(buffer+2, (tm->tm_sec/2) |
                         (tm->tm_min << 5) |
                         (tm->tm_hour << 1));
}

static void settrack(struct trackinfo *t, unsigned char * trackbuffer)
{
  dos_settime(trackbuffer+0,  &t->recordingtime);
  setbeword32(trackbuffer+4,  t->ekbnum);
  setbeword16(trackbuffer+8,  t->title);
  setbeword16(trackbuffer+10, t->artist);
  setbeword16(trackbuffer+12, t->album);
  trackbuffer[14] = t->trackinalbum;

  memcpy(trackbuffer+16, t->key, 8);
  memcpy(trackbuffer+24, t->mac, 8);
  trackbuffer[32] = t->codec_id;

  memcpy(trackbuffer+33, t->codecinfo, 3);
  memcpy(trackbuffer+44, t->codecinfo+3, 2);

  setbeword16(trackbuffer+36, t->firstfrag);
  setbeword16(trackbuffer+38, t->tracknum);
  setbeword16(trackbuffer+40, t->seconds);

  memcpy(trackbuffer+48,      t->contentid, 20);
  dos_settime(trackbuffer+68, &t->starttime);
  dos_settime(trackbuffer+72, &t->endtime);

  /* DRM stuff */
  trackbuffer[42] = t->Lt;
  trackbuffer[43] = t->Dest;
  trackbuffer[76] = t->Xcc;
  trackbuffer[78] = t->Cc;
}

static void setfrag(struct fraginfo *f, unsigned char * fragbuffer)
{
  memcpy(fragbuffer, &f->key, 8);
  setbeword16(fragbuffer+8,  f->firstblock);
  setbeword16(fragbuffer+10, f->lastblock);
  fragbuffer[12] = f->firstframe;
  fragbuffer[13] = f->lastframe;
  setbeword16(fragbuffer+14,f->nextfrag);
}

int himd_get_free_trackindex(struct himd * himd)
{
    int idx_freeslot;
    unsigned char * linkbuffer;

    linkbuffer   = get_track(himd, 0);
    idx_freeslot = beword16(&linkbuffer[38]);

    return idx_freeslot;
}

int himd_get_track_info(struct himd * himd, unsigned int idx, struct trackinfo * t, struct himderrinfo * status)
{
    unsigned char * trackbuffer;
    unsigned int firstpart;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(idx >= HIMD_FIRST_TRACK, -1);
    g_return_val_if_fail(idx <= HIMD_LAST_TRACK, -1);
    g_return_val_if_fail(t != NULL, -1);

    trackbuffer = get_track(himd, idx);
    firstpart = beword16(trackbuffer+36);

    if(firstpart == 0)
    {
        set_status_printf(status, HIMD_ERROR_NO_SUCH_TRACK,
                          _("Track %d is not present on disc"), idx);
        return -1;
    }
    get_dostime(&t->recordingtime,trackbuffer+0);
    t->ekbnum = beword32(trackbuffer+4);
    t->title = beword16(trackbuffer+8);
    t->artist = beword16(trackbuffer+10);
    t->album = beword16(trackbuffer+12);
    t->trackinalbum = trackbuffer[14];
    memcpy(t->key, trackbuffer+16,8);
    memcpy(t->mac, trackbuffer+24,8);
    t->codec_id = trackbuffer[32];
    memcpy(t->codecinfo,trackbuffer+33,3);
    memcpy(t->codecinfo+3,trackbuffer+44,2);
    t->firstfrag = firstpart;
    t->tracknum = beword16(trackbuffer+38);
    t->seconds = beword16(trackbuffer+40);
    memcpy(t->contentid,trackbuffer+48,20);
    get_dostime(&t->starttime,trackbuffer+68);
    get_dostime(&t->endtime,trackbuffer+72);
    return 0;
}


int himd_add_track_info(struct himd * himd, struct trackinfo * t, struct himderrinfo * status)
{
    int idx_freeslot;
    unsigned char * linkbuffer;
    unsigned char * trackbuffer;
    unsigned char * play_order_table = himd->tifdata+0x100;

    status = status;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(t != NULL, -1);

    /* get track[0] - the free-chain index */
    linkbuffer   = get_track(himd, 0);
    idx_freeslot = beword16(&linkbuffer[38]);

    /* allocate slot idx_freeslot for the new track*/
    trackbuffer  = get_track(himd, idx_freeslot);
    t->tracknum  = idx_freeslot;

    /* update track[] - free-chain index */
    setbeword16(&linkbuffer[38], beword16(&trackbuffer[38]));

    /* copy trackinfo to slot */
    settrack(t, trackbuffer);

    /* increase track count */
    setbeword16(play_order_table, himd_track_count(himd)+1);

    /* add entry for new track in play order table */
    setbeword16(play_order_table+2*idx_freeslot, t->tracknum);
    return idx_freeslot;
}


const char * himd_get_codec_name(const struct trackinfo * track)
{
    static char buffer[5];

    g_return_val_if_fail(track != NULL, "(nullptr)");

    if(track->codec_id == CODEC_LPCM)
        return "LPCM";
    if(track->codec_id == CODEC_ATRAC3)
        return "AT3 ";
    if(track->codec_id == CODEC_ATRAC3PLUS_OR_MPEG && 
         (track->codecinfo[0] & 3) == 0)
        return "AT3+";
    if(track->codec_id == CODEC_ATRAC3PLUS_OR_MPEG)
        return "MPEG";
    sprintf(buffer,"%4d",track->codec_id);
    return buffer;
}

unsigned int himd_trackinfo_framesize(const struct trackinfo * track)
{
    g_return_val_if_fail(track != NULL, 0);

    if(track->codec_id == CODEC_LPCM)
        return HIMD_LPCM_FRAMESIZE;
    if(track->codec_id == CODEC_ATRAC3)
        return 8 * track->codecinfo[2];
    if(track->codec_id == CODEC_ATRAC3PLUS_OR_MPEG &&
         (track->codecinfo[0] & 3) == 0)
        return 8 * (track->codecinfo[2] + 1);
    /* MP3 tracks don't have a fixed frame size, other track types unknown */
    return 0;
}

unsigned int himd_trackinfo_framesperblock(const struct trackinfo * track)
{
    int framesize;

    g_return_val_if_fail(track != NULL, 0);

    framesize = himd_trackinfo_framesize(track);
    if(!framesize)
        return TRACK_IS_MPEG;

    if(track->codec_id == CODEC_LPCM)
        return 0x3FC0 / HIMD_LPCM_FRAMESIZE;
    else
        return 0x3FBF / framesize;

    /* other track types unknown */
    return 0;
}

int himd_track_uploadable(struct himd * himd, const struct trackinfo * track)
{
    g_return_val_if_fail(himd != NULL, 0);
    g_return_val_if_fail(track != NULL, 0);

    /* MPEG has no serious encryption */
    if(track->codec_id == CODEC_ATRAC3PLUS_OR_MPEG &&
       (track->codecinfo[0] & 3) == 3)
        return 1;

    /* Not the well-known RH1 key */
    if(memcmp(track->key,"\0\0\0\0\0\0\0",8) != 0 ||
       track->ekbnum != 0x10012)
        return 0;

    return 1;
}

int himd_track_blocks(struct himd * himd, const struct trackinfo * track, struct himderrinfo * status)
{
    struct fraginfo frag;
    int fragnum, blocks = 0;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(track != NULL, -1);

    for(fragnum = track->firstfrag; fragnum != 0; fragnum = frag.nextfrag)
    {
        if(himd_get_fragment_info(himd, fragnum, &frag, status) < 0)
            return -1;
        blocks += frag.lastblock - frag.firstblock + 1;
    }
    return blocks;
}

int himd_get_fragment_info(struct himd * himd, unsigned int idx, struct fraginfo * f, struct himderrinfo * status)
{
    unsigned char * fragbuffer;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(idx >= HIMD_FIRST_FRAGMENT, -1);
    g_return_val_if_fail(idx <= HIMD_LAST_FRAGMENT, -1);
    g_return_val_if_fail(f != NULL, -1);

    fragbuffer = get_frag(himd, idx);
    memcpy(f->key, fragbuffer, 8);
    f->firstblock = beword16(fragbuffer + 8);
    f->lastblock = beword16(fragbuffer + 10);
    f->firstframe = fragbuffer[12];
    f->lastframe = fragbuffer[13];
    f->fragtype = fragbuffer[14] >> 4;
    f->nextfrag = beword16(fragbuffer+14) & 0xFFF;
    (void)status;
    return 0;
}


int himd_add_fragment_info(struct himd * himd, struct fraginfo * f, struct himderrinfo * status)
{
    int idx_freefrag;
    unsigned char * linkbuffer;
    unsigned char * fragbuffer;
    status = status;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(f != NULL, -1);

    linkbuffer    = get_frag(himd, 0);

    idx_freefrag  = beword16(linkbuffer+14) & 0xFFF;
    fragbuffer    = get_frag(himd, idx_freefrag);

    setbeword16(linkbuffer+14, beword16(fragbuffer+14) & 0xFFF);
    f->nextfrag = 0;

    /* copy fragment struct to slot buffer */
    setfrag(f, fragbuffer);

    return idx_freefrag;
}


char* himd_get_string_raw(struct himd * himd, unsigned int idx, int*type, int* length, struct himderrinfo * status)
{
    int curidx;
    int len;
    char * rawstr;
    int actualtype;

    g_return_val_if_fail(himd != NULL, NULL);
    g_return_val_if_fail(idx >= 1, NULL);
    g_return_val_if_fail(idx < 4096, NULL);
    
    actualtype = strtype(get_strchunk(himd,idx));
    /* Not the head of a string */
    if(actualtype < 8)
    {
        set_status_printf(status, HIMD_ERROR_NOT_STRING_HEAD,
                   _("String table entry %d is not a head: Type %d"),
                   idx,actualtype);
        return NULL;
    }
    if(type != NULL)
        *type = actualtype;

    /* Get length of string */
    len = 1;
    for(curidx = strlink(get_strchunk(himd,idx)); curidx != 0; 
          curidx = strlink(get_strchunk(himd,curidx)))
    {
        if(strtype(get_strchunk(himd,curidx)) != STRING_TYPE_CONTINUATION)
        {
            set_status_printf(status, HIMD_ERROR_STRING_CHAIN_BROKEN,
                       _("%dth entry in string chain starting at %d has type %d"),
                       len+1,idx,strtype(get_strchunk(himd,curidx)));
            return NULL;
        }
        len++;
        if(len >= 4096)
        {
            set_status_printf(status, HIMD_ERROR_STRING_CHAIN_BROKEN,
                       _("string chain starting at %d loops"),idx);
            return NULL;
        }
    }

    /* collect fragments */
    rawstr = g_malloc(len*14);
    if(!rawstr)
    {
        set_status_printf(status, HIMD_ERROR_OUT_OF_MEMORY,
                   _("Can't allocate %d bytes for raw string (string idx %d)"),
                   len, idx);
        return NULL;
    }

    len = 0;
    for(curidx = idx; curidx != 0; 
          curidx = strlink(get_strchunk(himd,curidx)))
    {
        memcpy(rawstr+len*14,get_strchunk(himd,curidx),14);
        len++;
    }

    *length = 14*len;
    return rawstr;
}

char* himd_get_string_utf8(struct himd * himd, unsigned int idx, int*type, struct himderrinfo * status)
{
    int length;
    char * out;
    char * srcencoding;
    char * rawstr;
    GError * err = NULL;
    g_return_val_if_fail(himd != NULL, NULL);
    g_return_val_if_fail(idx >= 1, NULL);
    g_return_val_if_fail(idx < 4096, NULL);
    
    rawstr = himd_get_string_raw(himd, idx, type, &length, status);
    if(!rawstr)
        return NULL;

    switch((unsigned char)rawstr[0])
    {
        case HIMD_ENCODING_LATIN1:
            srcencoding = "ISO-8859-1";
            break;
        case HIMD_ENCODING_UTF16BE:
            srcencoding = "UTF-16BE";
            break;
        case HIMD_ENCODING_SHIFT_JIS:
            srcencoding = "SHIFT_JIS";
            break;
        default:
            set_status_printf(status, HIMD_ERROR_UNKNOWN_ENCODING,
                       "string %d has unknown encoding with ID %d",
                       idx, rawstr[0]);
            himd_free(rawstr);
            return NULL;
    }
    out = g_convert(rawstr+1,length-1,"UTF-8",srcencoding,NULL,NULL,&err);
    himd_free(rawstr);
    if(err)
    {
        set_status_printf(status, HIMD_ERROR_STRING_ENCODING_ERROR,
                   "convert string %d from %s to UTF-8: %s",
                   idx, srcencoding, err->message);
        return NULL;
    }
    return out;
}


int himd_add_string(struct himd * himd, char *string, int type, int length, struct himderrinfo * status)
{
    int curidx, lastidx;
    int stridx, end_stridx;
    int nslots=0, rest=0;
    int idx_freeslot;
    unsigned char * linkchunk;
    unsigned char * freeslot;
    unsigned char * curchunk;

    const char * current_charset;
    int strencoding=0;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(string != NULL, -1);


    if(g_get_charset(&current_charset))
	strencoding = HIMD_ENCODING_LATIN1;
    else if(g_ascii_strncasecmp(current_charset, "UTF16BE", 7) == 0)
	strencoding = HIMD_ENCODING_UTF16BE;
    else if(g_ascii_strncasecmp(current_charset, "SHIFT_JIS", 9) == 0)
	strencoding = HIMD_ENCODING_SHIFT_JIS;
    else
	set_status_printf(status, HIMD_ERROR_UNKNOWN_ENCODING,
			  "unknown encoding");

    /* how many number of slots to store string in? */

    if(length <= 13)
	{
	    nslots = 1;
	}
    else if(length > 13)
	{
	    nslots = 1;
	    nslots += (length-13) / 14;
	    rest   = (length-13) % 14;
	    if(rest)
		nslots+=1;
	}
    else
	{
	}

    /* get index to first free slot in array */
    linkchunk = get_strchunk(himd, 0);
    idx_freeslot = strlink(linkchunk);

    /* update index link to next free chunk  */
    setbeword16(&linkchunk[14], idx_freeslot+nslots);

    /* head slot of where string will be stored */
    freeslot = get_strchunk(himd, idx_freeslot);

    /* indexes  to keep track of copying string into slots
       (slots[1],...,slot[N]; (N-1) nr of slots) */

    curidx     = idx_freeslot;
    lastidx    = (idx_freeslot + nslots) - 1;
    stridx     = 0;
    end_stridx = nslots;

    g_return_val_if_fail(curidx > 0, -1);
    g_return_val_if_fail(curidx < 4096, -1);
    g_return_val_if_fail(lastidx < 4096, -1);

    /* need the string any continuation slots ?*/
    if(length <= 13)
	{
	    curchunk    = get_strchunk(himd, curidx);
	    curchunk[0] = strencoding;

	    setbeword16(&curchunk[14], (type << 12) + 0x00);      // type | 00
	    memcpy(curchunk+1, &string[0], length);

	    return idx_freeslot;
	}
    /* nslots-1 continuation slots is needed */
    else
	{
	    /*
	       slot-head
	       nslots > 1 &&  (rest == 0 || rest > 0)
	    */
	    curchunk    = get_strchunk(himd, curidx);
	    curchunk[0] = strencoding;

	    setbeword16(&curchunk[14], (type << 12) + (curidx+1));      // type | lnk
	    memcpy(curchunk+1, &string[0], 13);

	    curidx   += 1;
	    stridx   += 1;
	}

    for (; curidx < (lastidx+1) && (stridx < end_stridx);
	 curidx += 1, stridx += 1)
	{
	    curchunk = get_strchunk(himd, curidx);

	    /* reached the last continuation slot ? */
	    if(curidx == lastidx)
		{
		    setbeword16(&curchunk[14], (STRING_TYPE_CONTINUATION << 12) + 0);      // type | lnk
		    memcpy(curchunk, &string[stridx*14], (rest == 0 ? 14 : rest) );
		    break;
		}

	    setbeword16(&curchunk[14], (STRING_TYPE_CONTINUATION << 12)+(curidx+1));      // type | lnk
	    memcpy(curchunk, &string[stridx*14], 14);
	}

    return idx_freeslot;
}
