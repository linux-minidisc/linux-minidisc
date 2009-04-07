#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "himd.h"

#define _(x) (x)

static int scanfortrkidx(GDir * dir)
{
    const char * hmafile;
    /* I don't use g_pattern_* stuff, because they can't be case insensitive */
    int datanum = -1;
    while((hmafile = g_dir_read_name(dir)) != NULL)
    {
        /* trkidxNN.hma - should be only one of them */
        if(g_strncasecmp(hmafile,"trkidx",6) == 0 &&
           strlen(hmafile) == 12 &&
           isdigit(hmafile[6]) &&
           isdigit(hmafile[7]) &&
           g_strncasecmp(hmafile+8,".hma",4) == 0)
        {
            sscanf(hmafile+6,"%d",&datanum);
            break;
        }
    }
    return datanum;
}

static unsigned int beword16(unsigned char * c)
{
    return c[0]*256+c[1];
}

static unsigned char * get_track(struct himd * himd, unsigned int idx)
{
    return himd->tifdata +  0x8000 + 0x50 * idx;
}

static unsigned char * get_strchunk(struct himd * himd, unsigned int idx)
{
    return himd->tifdata + 0x40000 + 0x10 * idx;
}

int himd_get_track_info(struct himd * himd, unsigned int idx, struct trackinfo * t)
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
        himd->status = HIMD_ERROR_NO_SUCH_TRACK;
        g_snprintf(himd->statusmsg, sizeof himd->statusmsg, _("Track %d is not present on disc"), idx);
        return -1;
    }
    t->title = beword16(trackbuffer+8);
    t->artist = beword16(trackbuffer+10);
    t->album = beword16(trackbuffer+12);
    t->trackinalbum = trackbuffer[14];
    t->codec_id = trackbuffer[32];
    memcpy(t->codecinfo,trackbuffer+33,3);
    memcpy(t->codecinfo+3,trackbuffer+44,2);
    t->firstpart = firstpart;
    t->tracknum = beword16(trackbuffer+38);
    t->seconds = beword16(trackbuffer+40);
    return 0;
}

static int himd_read_discid(struct himd * himd)
{
    char mclistname[13];
    FILE * mclistfile;
    char * filepath;

    sprintf(mclistname,"mclist%02d.hma",himd->datanum);
    filepath = g_build_filename(himd->rootpath,"hmdhifi",mclistname,NULL);
    mclistfile = fopen(filepath,"rb");
    g_free(filepath);

    if(!mclistfile)
    {
        himd->status = HIMD_ERROR_CANT_OPEN_MCLIST;
        g_snprintf(himd->statusmsg, sizeof himd->statusmsg, _("Can't open mclist file: %s\n"), g_strerror(errno));
        return -1;
    }

    fseek(mclistfile,0x40L,SEEK_SET);
    if(fread(himd->discid,16,1,mclistfile) != 1)
    {
        himd->status = HIMD_ERROR_CANT_READ_MCLIST;
        g_snprintf(himd->statusmsg, sizeof himd->statusmsg, _("Can't read mclist file: %s\n"), g_strerror(errno));
        fclose(mclistfile);
        return -1;
    }
    fclose(mclistfile);
    himd->discid_valid = 1;
    return 0;
}

int himd_open(struct himd * himd, const char * himdroot)
{
    char * filepath;
    char indexfilename[13];
    gsize filelen;
    GDir * dir;
    GError * error = NULL;
    
    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(himdroot != NULL, -1);

    filepath = g_build_filename(himdroot,"hmdhifi",NULL);
    dir = g_dir_open(filepath,0,NULL);
    g_free(filepath);
    if(dir == NULL)
    {
        himd->status = HIMD_ERROR_CANT_ACCESS_HMDHIFI;
        g_strlcpy(himd->statusmsg, error->message, sizeof himd->statusmsg);
        return -1;
    }

    himd->datanum = scanfortrkidx(dir);
    g_dir_close(dir);
    if(himd->datanum == -1)
    {
        himd->status = HIMD_ERROR_NO_TRACK_INDEX;
        g_strlcpy(himd->statusmsg, _("No track index file found"), sizeof himd->statusmsg);
        return -1;		/* ERROR: track index not found */
    }
    
    sprintf(indexfilename,"trkidx%02d.hma",himd->datanum);
    filepath = g_build_filename(himdroot,"hmdhifi",indexfilename,NULL);
    if(!g_file_get_contents(filepath, (char**)&himd->tifdata, &filelen, &error))
    {
        himd->status = HIMD_ERROR_CANT_READ_TIF;
        g_snprintf(himd->statusmsg,sizeof himd->statusmsg,_("Can't load TIF data from %s: %s"),filepath, error->message);
        g_free(filepath);
        return -1;
    }
    g_free(filepath);
    
    if(filelen != 0x50000)
    {
        himd->status = HIMD_ERROR_WRONG_TIF_SIZE;
        g_snprintf(himd->statusmsg,sizeof himd->statusmsg,_("TIF file is 0x%x bytes instead of 0x50000"),(int)filelen);
        g_free(himd->tifdata);
        return -1;
    }

    if(memcmp(himd->tifdata,"TIF ",4) != 0)
    {
        himd->status = HIMD_ERROR_WRONG_TIF_MAGIC;
        g_snprintf(himd->statusmsg,sizeof himd->statusmsg,_("TIF file starts with wrong magic: %02x %02x %02x %02x"),
                    himd->tifdata[0],himd->tifdata[1],himd->tifdata[2],himd->tifdata[3]);
        g_free(himd->tifdata);
        return -1;
    }

    himd->rootpath = g_strdup(himdroot);
    himd->status = HIMD_OK;
    himd->discid_valid = 0;

    return 0;
}

const unsigned char * himd_get_discid(struct himd * himd)
{
    if(!himd->discid_valid && himd_read_discid(himd) < 0)
        return 0;
    return himd->discid;
}

void himd_close(struct himd * himd)
{
    g_free(himd->tifdata);
    g_free(himd->rootpath);
}

void himd_free(void * data)
{
    g_free(data);
}

static int strtype(unsigned char * stringchunk)
{
    return stringchunk[14] >> 4;
}

static int strlink(unsigned char * stringchunk)
{
    return beword16(stringchunk+14) & 0xFFF;
}

char* himd_get_string_raw(struct himd * himd, unsigned int idx, int*type, int* length)
{
    int curidx;
    int len;
    char * rawstr;
    int actualtype;
    
    actualtype = strtype(get_strchunk(himd,idx));
    /* Not the head of a string */
    if(actualtype < 8)
    {
        himd->status = HIMD_ERROR_NOT_STRING_HEAD;
        g_snprintf(himd->statusmsg,sizeof(himd->statusmsg),
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
            himd->status = HIMD_ERROR_STRING_CHAIN_BROKEN;
            g_snprintf(himd->statusmsg,sizeof(himd->statusmsg),
                       "%dth entry in string chain starting at %d has type %d",
                       len+1,idx,strtype(get_strchunk(himd,curidx)));
            return NULL;
        }
        len++;
        if(len >= 4096)
        {
            himd->status = HIMD_ERROR_STRING_CHAIN_BROKEN;
            g_snprintf(himd->statusmsg,sizeof(himd->statusmsg),
                       "string chain starting at %d loops",idx);
            return NULL;
        }
    }

    /* collect fragments */
    rawstr = g_malloc(len*14);
    if(!rawstr)
    {
        himd->status = HIMD_ERROR_OUT_OF_MEMORY;
        g_snprintf(himd->statusmsg,sizeof(himd->statusmsg),
                   "Can't allocate %d bytes for raw string (string idx %d)",
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

char* himd_get_string_utf8(struct himd * himd, unsigned int idx, int*type)
{
    int length;
    char * out;
    char * srcencoding;
    char * rawstr;
    GError * err = NULL;
    g_return_val_if_fail(idx >= 1, NULL);
    g_return_val_if_fail(idx < 4096, NULL);
    
    rawstr = himd_get_string_raw(himd, idx, type, &length);
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
            himd->status = HIMD_ERROR_UNKNOWN_ENCODING;
            g_snprintf(himd->statusmsg,sizeof(himd->statusmsg),
                       "string %d has unknown encoding with ID %d",
                       idx, rawstr[0]);
            free(rawstr);
            return NULL;
    }
    out = g_convert(rawstr+1,length-1,"UTF-8",srcencoding,NULL,NULL,&err);
    g_free(rawstr);
    if(err)
    {
        himd->status = HIMD_ERROR_STRING_ENCODING_ERROR;
        g_snprintf(himd->statusmsg,sizeof(himd->statusmsg),
                   "convert string %d from %s to UTF-8: %s",
                   idx, srcencoding, err->message);
        return NULL;
    }
    return out;
}
