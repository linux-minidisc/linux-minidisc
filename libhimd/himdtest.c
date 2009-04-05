#include <stdio.h>
#include <glib.h>
#include <locale.h>

#include "himd.h"

void himd_stringdump(struct himd * himd)
{
    int i;
    for(i = 1;i < 4096;i++)
    {
        char * str;
        int type;
        if((str = himd_get_string_utf8(himd, i, &type)) != NULL)
        {
            char * typestr;
            char * outstr;
            switch(type)
            {
                case STRING_TYPE_TITLE: typestr="Title"; break;
                case STRING_TYPE_ARTIST: typestr="Artist"; break;
                case STRING_TYPE_ALBUM: typestr="Album"; break;
                case STRING_TYPE_GROUP: typestr="Group"; break;
                default: typestr=""; break;
            }
            outstr = g_locale_from_utf8(str,-1,NULL,NULL,NULL);
            printf("%4d: %-6s %s\n", i, typestr, outstr);
            g_free(outstr);
            himd_free(str);
        }
        else if(himd->status != HIMD_ERROR_NOT_STRING_HEAD)
            printf("%04d: ERROR %s\n", i, himd->statusmsg);
    }
}

int main(int argc, char ** argv)
{
    struct himd * h;
    setlocale(LC_ALL,"");
    if(argc < 2)
    {
        fputs("Please specify mountpoint of image\n",stderr);
        return 1;
    }
    h = himd_new(argv[1]);
    if(h->status != HIMD_OK)
        puts(h->statusmsg);
    else
    {
        himd_stringdump(h);
    }
    return 0;
}