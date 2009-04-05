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

static int read_tracks(struct himd * himd, FILE * idxfile)
{
    int i;
    unsigned char trackbuffer[64];

    fseek(idxfile,0x8000L, SEEK_SET);
    for(i = 0;i < 2048;i++)
    {
        struct trackinfo * t = &himd->tracks[i];
        if(fread(trackbuffer, 64, 1, idxfile) != 1)
        {
            himd->status = HIMD_ERROR_CANT_READ_TRACKS;
            g_snprintf(himd->statusmsg, sizeof himd->statusmsg, _("Can't read track %d info"), i);
            return -1;
        }
        t->title = beword16(trackbuffer+8);
        t->artist = beword16(trackbuffer+10);
        t->album = beword16(trackbuffer+12);
        t->trackinalbum = trackbuffer[14];
        t->codec_id = trackbuffer[32];
        memcpy(t->codecinfo,trackbuffer+33,3);
        memcpy(t->codecinfo+3,trackbuffer+44,2);
        t->firstpart = beword16(trackbuffer+36);
        t->tracknum = beword16(trackbuffer+38);
        t->seconds = beword16(trackbuffer+40);
    }
    return 0;
}

static int read_parts(struct himd * himd, FILE * idxfile)
{
    int i;
    unsigned char partbuffer[16];

    fseek(idxfile, 0x30000L, SEEK_SET);
    for(i = 0;i < 4096;i++)
    {
        struct partinfo * p = &himd->parts[i];
        if(fread(partbuffer, sizeof partbuffer, 1, idxfile) != 1)
        {
            himd->status = HIMD_ERROR_CANT_READ_PARTS;
            g_snprintf(himd->statusmsg, sizeof himd->statusmsg, _("Can't read part %d info"), i);
            return -1;
        }
        p->firstblock = beword16(partbuffer+8);
        p->lastblock = beword16(partbuffer+10);
        p->firstframe = partbuffer[12];
        p->lastframe = partbuffer[13];
        p->nextpart = beword16(partbuffer+14);
    }
    return 0;
}

static int read_strings(struct himd * himd, FILE * idxfile)
{
    int i;
    unsigned char strbuffer[16];

    fseek(idxfile, 0x40000L, SEEK_SET);
    for(i = 0;i < 4096;i++)
    {
        struct himdstring * s = &himd->strings[i];
        if(fread(strbuffer, sizeof strbuffer, 1, idxfile) != 1)
        {
            himd->status = HIMD_ERROR_CANT_READ_STRINGS;
            g_snprintf(himd->statusmsg, sizeof himd->statusmsg, _("Can't read string %d"), i);
            return -1;
        }
        memcpy(s->data,strbuffer,14);
        s->stringtype = strbuffer[14] >> 4;
        s->nextstring = beword16(strbuffer+14) & 0xFFF;
    }
    return 0;
}

int himd_open(struct himd * himd, const char * himdroot)
{
    char * filepath;
    char indexfilename[13];
    GDir * dir;
    GError * error = NULL;
    FILE * idxfile;
    
    int i;

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
    idxfile = fopen(filepath,"rb");
    g_free(filepath);

    if(!idxfile)
    {
        himd->status = HIMD_ERROR_CANT_OPEN_TRACK_INDEX;
        g_snprintf(himd->statusmsg, sizeof himd->statusmsg, _("Can't open track index file: %s\n"), g_strerror(errno));
        return -1;
    }
    
    i = read_tracks(himd, idxfile);
    if(i < 0)
        return -1;

    i = read_parts(himd, idxfile);
    if(i < 0)
        return -1;

    i = read_strings(himd, idxfile);
    if(i < 0)
        return -1;
    
    fclose(idxfile);

    himd->rootpath = g_strdup(himdroot);
    himd->status = HIMD_OK;

    return 0;
}

void himd_free(void * data)
{
    g_free(data);
}

char* himd_get_string_utf8(struct himd * himd, unsigned int idx, int*type)
{
    int curidx;
    int len;
    char * out;
    GError * err = NULL;
    char * srcencoding;
    unsigned char * tempstr;
    g_return_val_if_fail(idx >= 1, NULL);
    g_return_val_if_fail(idx < 4096, NULL);
    
    /* Not the head of a string */
    if(himd->strings[idx].stringtype < 8)
    {
        himd->status = HIMD_ERROR_NOT_STRING_HEAD;
        g_snprintf(himd->statusmsg,sizeof(himd->statusmsg),
                   _("String table entry %d is not a head: Type %d"),
                   idx,himd->strings[idx].stringtype);
        return NULL;
    }
    if(type != NULL)
        *type = himd->strings[idx].stringtype;

    /* Get length of string */
    len = 1;
    for(curidx = himd->strings[idx].nextstring; curidx != 0; 
          curidx = himd->strings[curidx].nextstring)
    {
        if(himd->strings[curidx].stringtype != STRING_TYPE_CONTINUATION)
        {
            himd->status = HIMD_ERROR_STRING_CHAIN_BROKEN;
            g_snprintf(himd->statusmsg,sizeof(himd->statusmsg),
                       "%dth entry in string chain starting at %d has type %d",
                       len+1,idx,himd->strings[curidx].stringtype);
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
    tempstr = malloc(len*14);
    if(!tempstr)
    {
        himd->status = HIMD_ERROR_OUT_OF_MEMORY;
        g_snprintf(himd->statusmsg,sizeof(himd->statusmsg),
                   "Can't allocate %d bytes for temp string buffer (string idx %d)",
                   len, idx);
        return NULL;
    }

    len = 0;
    for(curidx = idx; curidx != 0; 
          curidx = himd->strings[curidx].nextstring)
    {
        memcpy(tempstr+len*14,himd->strings[curidx].data,14);
        len++;
    }
    switch(tempstr[0])
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
                       idx, tempstr[0]);
            free(tempstr);
            return NULL;
    }
    out = g_convert(tempstr+1,len*14-1,"UTF-8",srcencoding,NULL,NULL,&err);
    free(tempstr);
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

struct himd * himd_new(const char * himdroot)
{
    struct himd * himd;
    himd = malloc(sizeof(*himd));
    if(!himd)
        return NULL;
    himd_open(himd, himdroot);
    return himd;
}
