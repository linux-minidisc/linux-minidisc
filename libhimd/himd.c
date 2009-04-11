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
    int maxdatanum = -1;
    int curdatanum;
    while((hmafile = g_dir_read_name(dir)) != NULL)
    {
        /* trkidxNN.hma - should be only one of them */
        if(g_strncasecmp(hmafile,"trkidx",6) == 0 &&
           strlen(hmafile) == 12 &&
           isxdigit(hmafile[6]) &&
           isxdigit(hmafile[7]) &&
           g_strncasecmp(hmafile+8,".hma",4) == 0 &&
           sscanf(hmafile+6,"%x",&curdatanum) == 1 &&
           curdatanum > maxdatanum)
            maxdatanum = curdatanum;
    }
    return maxdatanum;
}

FILE * himd_open_file(struct himd * himd, const char * fileid)
{
    char filename[13];
    FILE * file;
    char * filepath;

    sprintf(filename,"%s%02x.hma",fileid,himd->datanum);
    filepath = g_build_filename(himd->rootpath,"hmdhifi",filename,NULL);
    file = fopen(filepath,"rb");
    g_free(filepath);
    return file;
}

static int himd_read_discid(struct himd * himd)
{
    FILE * mclistfile = himd_open_file(himd, "mclist");

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
    dir = g_dir_open(filepath,0,&error);
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
    
    sprintf(indexfilename,"trkidx%02x.hma",himd->datanum);
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
