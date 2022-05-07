/*
 * trackindex.c
 *
 * This file is part of libhimd, a library for accessing Sony HiMD devices.
 *
 * Copyright (C) 2009-2011 Michael Karcher
 * Copyright (C) 2011 Mårten Cassel
 * Copyright (C) 2011 Thomas Arp
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <string.h>
#include <glib.h>
#include "himd.h"
#include "himdll.h"

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

int himdll_strtype(struct himd *himd, unsigned int idx)
{
    g_return_val_if_fail(idx < 4096, -1);
    return strtype(get_strchunk(himd, idx));
}

int himdll_strlink(struct himd *himd, unsigned int idx)
{
    g_return_val_if_fail(idx < 4096, -1);
    return strlink(get_strchunk(himd, idx));
}

static void set_strlink(unsigned char * stringchunk, int link)
{
    setbeword16(stringchunk+14, (beword16(stringchunk+14) & 0xF000) | link);
}

static void set_strtype(unsigned char * stringchunk, int type)
{
    stringchunk[14] = (stringchunk[14] & 0x0F) |  (type << 4);
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

static gboolean is_out_of_range(const struct tm * tm)
{
    return tm->tm_mday < 1 || tm->tm_mday > 31 ||
           tm->tm_mon < 0 || tm->tm_mon > 11 ||
           tm->tm_year < 80 || tm->tm_year > 207 ||
           tm->tm_sec < 0 || tm->tm_sec > 59 ||
           tm->tm_min < 0 || tm->tm_min > 59 ||
           tm->tm_hour < 0 || tm->tm_hour > 23 ;
}

static void dos_settime(unsigned char * buffer, const struct tm * tm)
{
   if(is_out_of_range(tm))
       memset(buffer, 0, 4);
   else
   {
       setbeword16(buffer, (tm->tm_mday) |
                           ((tm->tm_mon + 1) << 5) |
                           ((tm->tm_year - 80) << 9));
       setbeword16(buffer+2, (tm->tm_sec/2) |
                             (tm->tm_min << 5) |
                             (tm->tm_hour << 11));
   }
}

static void settrack(const struct trackinfo *t, unsigned char * trackbuffer)
{
  dos_settime(trackbuffer+0,  &t->recordingtime);
  setbeword32(trackbuffer+4,  t->ekbnum);
  setbeword16(trackbuffer+8,  t->title);
  setbeword16(trackbuffer+10, t->artist);
  setbeword16(trackbuffer+12, t->album);
  trackbuffer[14] = t->trackinalbum;

  memcpy(trackbuffer+16, t->key, 8);
  memcpy(trackbuffer+24, t->mac, 8);
  trackbuffer[32] = t->codec_info.codec_id;

  memcpy(trackbuffer+33, t->codec_info.codecinfo, 3);
  memcpy(trackbuffer+44, t->codec_info.codecinfo+3, 2);

  setbeword16(trackbuffer+36, t->firstfrag);
  setbeword16(trackbuffer+38, t->tracknum);
  setbeword16(trackbuffer+40, t->seconds);

  memcpy(trackbuffer+48,      t->contentid, 20);

  /* DRM stuff */
  dos_settime(trackbuffer+68, &t->licensestarttime);
  dos_settime(trackbuffer+72, &t->licenseendtime);
  trackbuffer[42] = t->lt;
  trackbuffer[43] = t->dest;
  trackbuffer[76] = t->xcc;
  trackbuffer[77] = t->ct;
  trackbuffer[78] = t->cc;
  trackbuffer[79] = t->cn;
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

int himdll_get_track_info(struct himd * himd, unsigned int idx, struct trackinfo * t, struct himderrinfo * status)
{
    unsigned char * trackbuffer;

    (void)status;
    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(idx >= HIMD_FIRST_TRACK, -1);
    g_return_val_if_fail(idx <= HIMD_LAST_TRACK, -1);
    g_return_val_if_fail(t != NULL, -1);

    trackbuffer = get_track(himd, idx);

    get_dostime(&t->recordingtime,trackbuffer+0);
    t->ekbnum = beword32(trackbuffer+4);
    t->title = beword16(trackbuffer+8);
    t->artist = beword16(trackbuffer+10);
    t->album = beword16(trackbuffer+12);
    t->trackinalbum = trackbuffer[14];
    memcpy(t->key, trackbuffer+16,8);
    memcpy(t->mac, trackbuffer+24,8);
    t->codec_info.codec_id = trackbuffer[32];
    memcpy(t->codec_info.codecinfo,trackbuffer+33,3);
    memcpy(t->codec_info.codecinfo+3,trackbuffer+44,2);
    t->firstfrag = beword16(trackbuffer+36);
    t->tracknum = beword16(trackbuffer+38);
    t->seconds = beword16(trackbuffer+40);
    t->lt = trackbuffer[42];
    t->dest = trackbuffer[43];
    memcpy(t->contentid,trackbuffer+48,20);
    get_dostime(&t->licensestarttime,trackbuffer+68);
    get_dostime(&t->licenseendtime,trackbuffer+72);
    t->xcc = trackbuffer[76];
    t->ct = trackbuffer[77];
    t->cc = trackbuffer[78];
    t->cn = trackbuffer[79];
    return 0;
}

int himd_get_track_info(struct himd * himd, unsigned int idx, struct trackinfo * t, struct himderrinfo * status)
{
    if(himdll_get_track_info(himd, idx, t, status) < 0)
        return -1;

    if(t->firstfrag == 0)
    {
        set_status_printf(status, HIMD_ERROR_NO_SUCH_TRACK,
                          _("Track %d is not present on disc"), idx);
        return -1;
    }
    return 0;
}

int himd_update_track_info(struct himd * himd, unsigned int idx, const struct trackinfo * track, struct himderrinfo * status)
{
    (void) status;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(idx >= HIMD_FIRST_TRACK, -1);
    g_return_val_if_fail(idx <= HIMD_LAST_TRACK, -1);
    g_return_val_if_fail(track != NULL, -1);

    settrack(track, get_track(himd, idx));

    return 0;
}

int himd_add_track_info(struct himd * himd, struct trackinfo * t, struct himderrinfo * status)
{
    int idx_freeslot;
    unsigned char * linkbuffer;
    unsigned char * trackbuffer;
    unsigned char * play_order_table = himd->tifdata+0x100;

    (void)status;

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


unsigned int himd_trackinfo_framesperblock(const struct trackinfo * track)
{
    int framesize;

    g_return_val_if_fail(track != NULL, 0);

    framesize = himd_trackinfo_framesize(track);
    if(!framesize)
        return TRACK_IS_MPEG;

    if(sony_codecinfo_is_lpcm(&track->codec_info))
        return 0x3FC0 / SONY_VIRTUAL_LPCM_FRAMESIZE;
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
    if(sony_codecinfo_is_mpeg(&track->codec_info))
        return 1;

    /* Not the well-known EKB */
    if(track->ekbnum != 0x10012)
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

    (void)status;

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


int himd_add_string(struct himd * himd, const char * string, int type, struct himderrinfo * status)
{
    int curidx, curtype, i, nextidx;
    int nslots;
    int idx_firstslot;
    gsize length;
    gchar * convertedstring;
    unsigned char * curchunk;
    unsigned char strencoding;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(string != NULL, -1);


    /* try to use Latin-1 or Shift-JIS. If that fails, use Unicode. */
    if((convertedstring = g_convert(string,-1,"ISO-8859-1","UTF8",
                                    NULL,&length,NULL)) != NULL)
        strencoding = HIMD_ENCODING_LATIN1;
    else if((convertedstring = g_convert(string,-1,"SHIFT_JIS","UTF8",
                                    NULL,&length,NULL)) != NULL)
        strencoding = HIMD_ENCODING_SHIFT_JIS;
    else if((convertedstring = g_convert(string,-1,"UTF-16BE","UTF8",
                                    NULL,&length,NULL)) != NULL)
        strencoding = HIMD_ENCODING_UTF16BE;
    else {
        /* should never happen, as utf-16 can encode anything */
	set_status_printf(status, HIMD_ERROR_UNKNOWN_ENCODING,
			  "can't encode the string '%s' into anything usable",
                          string);
        return -1;
    }

    /* how many number of slots to store string in? */
    nslots = (length+14)/14;	/* +13 for rounding up, +1 for the encoding byte */

    /* check that there are enough free slots. Start at slot 0 which
       is the head of the free list. */
    curidx = 0;
    for(i = 0; i < nslots; i++)
    {
        curtype = strtype(get_strchunk(himd, curidx));
        curidx = strlink(get_strchunk(himd, curidx));
        if(!curidx)
        {
            g_free(convertedstring);
            set_status_printf(status, HIMD_ERROR_OUT_OF_STRINGS,
                "Not enough string space to allocate %d string slots\n", nslots);
            return -1;
        }
        if(curtype != STRING_TYPE_UNUSED)
        {
            g_free(convertedstring);
            set_status_printf(status, HIMD_ERROR_STRING_CHAIN_BROKEN,
                "String slot %d in free list has type %d\n", curidx, curtype);
            return -1;
        }
    }

    idx_firstslot = strlink(get_strchunk(himd, 0));
    curidx = idx_firstslot;
    for(i = 0; i < nslots; i++)
    {
        /* reserve space for the encoding byte in the first slot */
        gsize slotlen = (i != 0) ? 14 : 13;
        gsize stroffset = i*14 - 1;

        /* limit length to what is remaining of the string */
        if(slotlen > length - stroffset)
            slotlen = length - stroffset;

        curchunk = get_strchunk(himd, curidx);
        nextidx  = strlink(curchunk);
        if(i == 0)
        {
            curchunk[0] = strencoding;
            memcpy(curchunk + 1, convertedstring, slotlen);
            set_strtype(curchunk, type);
        }
        else
        {
            memcpy(curchunk, convertedstring + stroffset, slotlen);
            set_strtype(curchunk, STRING_TYPE_CONTINUATION);
        }
        if(i == nslots-1)
            set_strlink(curchunk, 0);
        curidx = nextidx;
    }

    /* adjust free list head pointer */
    set_strlink(get_strchunk(himd, 0), curidx);
    g_free(convertedstring);

    return idx_firstslot;
}

int himd_remove_string(struct himd * himd, unsigned int idx, struct himderrinfo * status)
{
    (void)status;

    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(idx < 4096, -1);

    unsigned char * freelist = get_strchunk(himd, 0);

    // Freelist points to the next free chunk
    int next_free_chunk = strlink(freelist);

    // Insert the current string into the beginning of the freelist
    set_strlink(freelist, idx);

    while (idx != 0) {
        unsigned char * curchunk = get_strchunk(himd, idx);

        // Clear the chunk
        memset(curchunk, 0, 14);
        set_strtype(curchunk, STRING_TYPE_UNUSED);

        idx = strlink(curchunk);

        if (idx == 0) {
            // Add the tail of the free list
            set_strlink(curchunk, next_free_chunk);
        }
    }

    return 0;
}

int himd_track_set_string(struct himd * himd, unsigned int idx, struct trackinfo * track, int string_type, const char * new_string_utf8, struct himderrinfo * status)
{
    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(idx >= HIMD_FIRST_TRACK, -1);
    g_return_val_if_fail(idx <= HIMD_LAST_TRACK, -1);
    g_return_val_if_fail(track != NULL, -1);

    int *pidx;

    switch (string_type) {
        case STRING_TYPE_TITLE:
            pidx = &track->title;
            break;
        case STRING_TYPE_ARTIST:
            pidx = &track->artist;
            break;
        case STRING_TYPE_ALBUM:
            pidx = &track->album;
            break;
        default:
            set_status_printf(status, HIMD_ERROR_STRING_ENCODING_ERROR, _("String type not supported"));
            return -1;
    }

    // Remove old string
    if (*pidx != 0) {
        int res = himd_remove_string(himd, *pidx, status);
        if (res == 0) {
            *pidx = 0;
        } else {
            return -1;
        }
    }

    // Add new string (if it's non-empty, otherwise just leave it at zero0
    if (new_string_utf8 != NULL && new_string_utf8[0] != '\0') {
        int res = himd_add_string(himd, new_string_utf8, string_type, status);
        if (res >= 0) {
            *pidx = res;
        } else {
            return -1;
        }
    }

    return himd_update_track_info(himd, idx, track, status);
}
