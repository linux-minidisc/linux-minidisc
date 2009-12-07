#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#define G_LOG_DOMAIN "HiMD"
#include <glib.h>
#include <glib/gprintf.h>

#include "himd.h"

#define _(x) (x)

void set_status_const(struct himderrinfo * status, enum himdstatus code, const char * msg)
{
    if(status)
    {
        status->status = code;
        g_strlcpy(status->statusmsg, msg, sizeof status->statusmsg);
    }
}

void set_status_printf(struct himderrinfo * status, enum himdstatus code, const char * format, ...)
{
    if(status)
    {
        va_list args;
        va_start(args, format);
        status->status = code;
        g_vsnprintf(status->statusmsg, sizeof status->statusmsg, format, args);
        va_end(args);
    }
}

static int scanforatdata(GDir * dir)
{
    const char * hmafile;
    /* I don't use g_pattern_* stuff, because they can't be case insensitive */
    int maxdatanum = -1;
    int curdatanum;
    while((hmafile = g_dir_read_name(dir)) != NULL)
    {
        /* atdataNN.hma - should be only one of them */
        if(g_ascii_strncasecmp(hmafile,"atdata0",7) == 0 &&
           strlen(hmafile) == 12 &&
           isxdigit(hmafile[7]) &&
           g_ascii_strncasecmp(hmafile+8,".hma",4) == 0 &&
           sscanf(hmafile+6,"%x",&curdatanum) == 1 &&
           curdatanum > maxdatanum)
        {
            if(maxdatanum != -1)
                g_warning("Found two atdata files: %02X and %02X\n",curdatanum,maxdatanum);
            maxdatanum = curdatanum;
        }
    }
    return maxdatanum;
}

static void nong_inplace_ascii_down(gchar * string)
{
    while(*string)
    {
        *string = g_ascii_tolower(*string);
        string++;
    }
}

static void nong_inplace_ascii_up(gchar * string)
{
    while(*string)
    {
        *string = g_ascii_toupper(*string);
        string++;
    }
}

FILE * himd_open_file(struct himd * himd, const char * fileid)
{
    char filename[13];
    FILE * file;
    char * filepath;

    sprintf(filename,"%s%02X.HMA",fileid,himd->datanum);
    if(himd->need_lowercase)
        nong_inplace_ascii_down(filename);
    else
        nong_inplace_ascii_up(filename);
    filepath = g_build_filename(himd->rootpath,himd->need_lowercase ? "hmdhifi" : "HMDHIFI",filename,NULL);
    file = fopen(filepath,"rb");
    g_free(filepath);
    return file;
}

static int himd_read_discid(struct himd * himd, struct himderrinfo * status)
{
    FILE * mclistfile = himd_open_file(himd, "MCLIST");

    if(!mclistfile)
    {
        set_status_printf(status, HIMD_ERROR_CANT_OPEN_MCLIST,
                          _("Can't open mclist file: %s\n"), g_strerror(errno));
        return -1;
    }

    fseek(mclistfile,0x40L,SEEK_SET);
    if(fread(himd->discid,16,1,mclistfile) != 1)
    {
        set_status_printf(status, HIMD_ERROR_CANT_READ_MCLIST,
                          _("Can't read mclist file: %s\n"), g_strerror(errno));
        fclose(mclistfile);
        return -1;
    }
    fclose(mclistfile);
    himd->discid_valid = 1;
    return 0;
}

int himd_open(struct himd * himd, const char * himdroot, struct himderrinfo * status)
{
    char * filepath;
    char indexfilename[13];
    gsize filelen;
    GDir * dir;
    GError * error = NULL;
    
    g_return_val_if_fail(himd != NULL, -1);
    g_return_val_if_fail(himdroot != NULL, -1);

    himd->need_lowercase = 0;
    filepath = g_build_filename(himdroot,"HMDHIFI",NULL);
    dir = g_dir_open(filepath,0,&error);
    if(g_error_matches(error,G_FILE_ERROR,G_FILE_ERROR_NOENT))
    {
        g_error_free(error);
        error = NULL;
        filepath = g_build_filename(himdroot,"hmdhifi",NULL);
        dir = g_dir_open(filepath,0,&error);
        himd->need_lowercase = 1;
    }
    g_free(filepath);
    if(dir == NULL)
    {
        set_status_const(status, HIMD_ERROR_CANT_ACCESS_HMDHIFI, error->message);
        return -1;
    }

    himd->datanum = scanforatdata(dir);
    g_dir_close(dir);
    if(himd->datanum == -1)
    {
        set_status_const(status, HIMD_ERROR_NO_TRACK_INDEX, _("No track index file found"));
        return -1;		/* ERROR: track index not found */
    }
    
    sprintf(indexfilename,
            himd->need_lowercase ? "trkidx%02x.hma" : "TRKIDX%02X.HMA",
            himd->datanum);
    filepath = g_build_filename(himdroot,himd->need_lowercase ? "hmdhifi" : 
                                "HMDHIFI",indexfilename,NULL);
    if(!g_file_get_contents(filepath, (char**)&himd->tifdata, &filelen, &error))
    {
        set_status_printf(status, HIMD_ERROR_CANT_READ_TIF,
                          _("Can't load TIF data from %s: %s"),
                          filepath, error->message);
        g_free(filepath);
        return -1;
    }
    g_free(filepath);
    
    if(filelen != 0x50000)
    {
        set_status_printf(status, HIMD_ERROR_WRONG_TIF_SIZE,
                          _("TIF file is 0x%x bytes instead of 0x50000"),
                          (int)filelen);
        g_free(himd->tifdata);
        return -1;
    }

    if(memcmp(himd->tifdata,"TIF ",4) != 0)
    {
        set_status_printf(status, HIMD_ERROR_WRONG_TIF_MAGIC,
                         _("TIF file starts with wrong magic: %02x %02x %02x %02x"),
                         himd->tifdata[0],himd->tifdata[1],himd->tifdata[2],himd->tifdata[3]);
        g_free(himd->tifdata);
        return -1;
    }

    himd->rootpath = g_strdup(himdroot);
    himd->discid_valid = 0;

    return 0;
}

const unsigned char * himd_get_discid(struct himd * himd, struct himderrinfo * status)
{
    if(!himd->discid_valid && himd_read_discid(himd, status) < 0)
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
