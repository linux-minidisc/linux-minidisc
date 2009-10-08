#include <stdio.h>
#include <glib.h>
#include <locale.h>
#include <string.h>

#include "himd.h"
#include "sony_oma.h"

static const char * hexdump(unsigned char * input, int len)
{
    static char dumpspace[5][41];
    static int dumpindex = 0;
    int i;

    if(len > 20) return "TOO LONG";

    dumpindex %= 5;
    for(i = 0;i < len;++i)
        sprintf(dumpspace[dumpindex]+i*2,"%02x",input[i]);

    return dumpspace[dumpindex++];
}

char * get_locale_str(struct himd * himd, int idx)
{
    char * str, * outstr;
    if(idx == 0)
        return NULL;

    str = himd_get_string_utf8(himd, idx, NULL, NULL);
    if(!str)
        return NULL;

    outstr = g_locale_from_utf8(str,-1,NULL,NULL,NULL);
    himd_free(str);
    return outstr;
}

void himd_trackdump(struct himd * himd, int verbose)
{
    int i;
    struct himderrinfo status;
    for(i = HIMD_FIRST_TRACK;i <= HIMD_LAST_TRACK;i++)
    {
        struct trackinfo t;
        if(himd_get_track_info(himd, i, &t, NULL)  >= 0)
        {
            char *title, *artist, *album;
            title = get_locale_str(himd, t.title);
            artist = get_locale_str(himd, t.artist);
            album = get_locale_str(himd, t.album);
            printf("%4d: %d:%02d %s %s:%s (%s %d)%s\n",
                    i, t.seconds/60, t.seconds % 60, himd_get_codec_name(&t),
                    artist ? artist : "Unknown artist", 
                    title ? title : "Unknown title",
                    album ? album : "Unknown album", t.trackinalbum,
                    himd_track_uploadable(himd, &t) ? " [uploadable]":"");
            g_free(title);
            g_free(artist);
            g_free(album);
            if(verbose)
            {
                struct fraginfo f;
                int fnum = t.firstfrag;
                int blocks;

                if((blocks = himd_track_blocks(himd, &t, &status)) < 0)
                    fprintf(stderr, "Can't get block count for Track %d: %s\n", i, status.statusmsg);
                else
                    printf("     %5d Blocks per 16 KB\n", blocks);

                while(fnum != 0)
                {
                    if(himd_get_fragment_info(himd, fnum, &f, &status) >= 0)
                    {
                        printf("     %3d@%05d .. %3d@%05d (%s)\n", f.firstframe, f.firstblock, f.lastframe, f.lastblock, hexdump(f.key, 8));
                        fnum = f.nextfrag;
                    }
                    else
                    {
                        printf("     ERROR reading fragment %d info: %s\n", fnum, status.statusmsg);
                        break;
                    }
                }
                printf("     Contend ID: %s\n", hexdump(t.contentid, 20));
                printf("     Key: %s (EKB %08x); MAC: %s\n", hexdump(t.key, 8), t.ekbnum, hexdump(t.mac, 8));
            }
        }
    }
}

void himd_stringdump(struct himd * himd)
{
    int i;
    struct himderrinfo status;
    for(i = 1;i < 4096;i++)
    {
        char * str;
        int type;
        if((str = himd_get_string_utf8(himd, i, &type, &status)) != NULL)
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
        else if(status.status != HIMD_ERROR_NOT_STRING_HEAD)
            printf("%04d: ERROR %s\n", i, status.statusmsg);
    }
}

void himd_dumpdiscid(struct himd * h)
{
    int i;
    struct himderrinfo status;
    const unsigned char * discid = himd_get_discid(h, &status);
    if(!discid)
    {
        fprintf(stderr,"Error obtaining disc ID: %s", status.statusmsg);
        return;
    }
    printf("Disc ID: ");
    for(i = 0;i < 16;++i)
        printf("%02X",discid[i]);
    puts("");        
}

void himd_dumptrack(struct himd * himd, int trknum)
{
    struct trackinfo t;
    struct himd_blockstream str;
    struct himderrinfo status;
    FILE * strdumpf;
    unsigned int firstframe, lastframe;
    unsigned char block[16384];
    int blocknum = 0;
    strdumpf = fopen("stream.dmp","wb");
    if(!strdumpf)
    {
        perror("Opening stream.dmp");
        return;
    }
    if(himd_get_track_info(himd, trknum, &t, &status) < 0)
    {
        fprintf(stderr, "Error obtaining track info: %s\n", status.statusmsg);
        return;
    }
    if(himd_blockstream_open(himd, t.firstfrag, himd_trackinfo_framesperblock(&t), &str, &status) < 0)
    {
        fprintf(stderr, "Error opening stream %d: %s\n", t.firstfrag, status.statusmsg);
        return;
    }
    while(himd_blockstream_read(&str, block, &firstframe, &lastframe, &status) >= 0)
    {
        if(fwrite(block,16384,1,strdumpf) != 1)
        {
            perror("writing dumped stream");
            goto clean;
        }
        printf("%d: %u..%u\n",blocknum++,firstframe,lastframe);
    }
    if(status.status != HIMD_STATUS_AUDIO_EOF)
        fprintf(stderr,"Error reading MP3 data: %s\n", status.statusmsg);
clean:
    fclose(strdumpf);
    himd_blockstream_close(&str);
}

void himd_dumpmp3(struct himd * himd, int trknum)
{
    struct himd_mp3stream str;
    struct himderrinfo status;
    FILE * strdumpf;
    unsigned int len;
    const unsigned char * data;
    strdumpf = fopen("stream.mp3","wb");
    if(!strdumpf)
    {
        perror("Opening stream.mp3");
        return;
    }
    if(himd_mp3stream_open(himd, trknum, &str, &status) < 0)
    {
        fprintf(stderr, "Error opening track %d: %s\n", trknum, status.statusmsg);
        return;
    }
    while(himd_mp3stream_read_frame(&str, &data, &len, &status) >= 0)
    {
        if(fwrite(data,len,1,strdumpf) != 1)
        {
            perror("writing dumped stream");
            goto clean;
        }
    }
    if(status.status != HIMD_STATUS_AUDIO_EOF)
        fprintf(stderr,"Error reading MP3 data: %s\n", status.statusmsg);
clean:
    fclose(strdumpf);
    himd_mp3stream_close(&str);
}


int write_oma_header(FILE * f, const struct trackinfo * trkinfo)
{
    char header[EA3_FORMAT_HEADER_SIZE];
    make_ea3_format_header(header, trkinfo);

    if(fwrite(header, sizeof header, 1, f) != 1)
    {
        perror("Writing OMA header");
        return -1;
    }
    return 0;
}

/* For LPCM: creates headerless PCM
             play with: play -t s2 -r 44100 -B -c2 stream.pcm
   For ATRAC3/ATRAC3+: creates a .oma file (with ea3 tag header)
             play with Sonic Stage (ffmpeg needs support of tagless files,
                                    ffmpeg does not support ATRAC3+)
 */
void himd_dumpnonmp3(struct himd * himd, int trknum)
{
    struct himd_nonmp3stream str;
    struct himderrinfo status;
    struct trackinfo trkinfo;
    FILE * strdumpf;
    const char * filename = "stream.pcm";
    unsigned int len;
    const unsigned char * data;
    if(himd_get_track_info(himd, trknum, &trkinfo, &status) < 0)
    {
        fprintf(stderr, "Error obtaining track info: %s\n", status.statusmsg);
        return;
    }

    if(trkinfo.codec_id != CODEC_LPCM)
        filename = "stream.oma";

    strdumpf = fopen(filename,"wb");
    if(!strdumpf)
    {
        fprintf(stderr, "opening ");
        perror(filename);
        return;
    }
    if(himd_nonmp3stream_open(himd, trknum, &str, &status) < 0)
    {
        fprintf(stderr, "Error opening track %d: %s\n", trknum, status.statusmsg);
        fclose(strdumpf);
        return;
    }
    if(trkinfo.codec_id != CODEC_LPCM &&
       write_oma_header(strdumpf, &trkinfo) < 0)
        return;
    while(himd_nonmp3stream_read_block(&str, &data, &len, NULL, &status) >= 0)
    {
        if(fwrite(data,len,1,strdumpf) != 1)
        {
            perror("writing dumped stream");
            goto clean;
        }
    }
    if(status.status != HIMD_STATUS_AUDIO_EOF)
        fprintf(stderr,"Error reading PCM data: %s\n", status.statusmsg);
clean:
    fclose(strdumpf);
    himd_nonmp3stream_close(&str);
}

int main(int argc, char ** argv)
{
    int idx;
    struct himd h;
    struct himderrinfo status;
    setlocale(LC_ALL,"");
    if(argc < 2)
    {
        fputs("Please specify mountpoint of image\n",stderr);
        return 1;
    }
    if(himd_open(&h,argv[1], &status) < 0)
    {
        puts(status.statusmsg);
        return 1;
    }
    if(argc == 2 || strcmp(argv[2],"strings") == 0)
        himd_stringdump(&h);
    else if(strcmp(argv[2],"tracks") == 0)
        himd_trackdump(&h, argc > 3);
    else if(strcmp(argv[2],"discid") == 0)
        himd_dumpdiscid(&h);
    else if(strcmp(argv[2],"mp3key") == 0 && argc > 3)
    {
        mp3key k;
        idx = 1;
        sscanf(argv[3], "%d", &idx);
        himd_obtain_mp3key(&h, idx, &k, NULL);
        printf("Track key: %02x%02x%02x%02x\n", k[0], k[1], k[2], k[3]);
    }
    else if(strcmp(argv[2],"dumptrack") == 0 && argc > 3)
    {
        idx = 1;
        sscanf(argv[3], "%d", &idx);
        himd_dumptrack(&h, idx);
    }
    else if(strcmp(argv[2],"dumpmp3") == 0 && argc > 3)
    {
        idx = 1;
        sscanf(argv[3], "%d", &idx);
        himd_dumpmp3(&h, idx);
    }
    else if(strcmp(argv[2],"dumpnonmp3") == 0 && argc > 3)
    {
        idx = 1;
        sscanf(argv[3], "%d", &idx);
        himd_dumpnonmp3(&h, idx);
    }

    himd_close(&h);
    return 0;
}
