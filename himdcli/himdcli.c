/*
 *   himdcli.c - list contents (tracks, holes), dump tracks and show diskid of a HiMD 
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <locale.h>
#include <string.h>
#include <glib/gstdio.h>
#include <json.h>

#include "libusbmd.h"
#include "himd.h"
#include "sony_oma.h"

static json_object *json;

void usage(char * cmdname)
{
  printf("Usage: %s <HiMD path> <command>, where <command> is either of:\n\n\
          usbids           - lists all USB VID/PIDs for Hi-MD devices as JSON (no <HiMD path>)\n\
          strings          - dumps all strings found in the tracklist file\n\
          tracks           - lists all tracks on disc\n\
          tracks verbose   - lists details of all tracks on disc\n\
          tracks json      - lists all tracks on disc in json format\n\
          discid           - reads the disc id of the inserted medium\n\
          holes            - lists all holes on disc\n\
          mp3key <TRK>     - show the MP3 encryption key for track <TRK>\n\
          dumptrack <TRK>  - dump track <TRK>\n\
          dumpmp3 <TRK>    - dump MP3 track <TRK>\n\
          dumpnonmp3 <TRK> - dump non-MP3 track <TRK>\n\
          writemp3 <FILE>  - write mp3 to disc\n", cmdname);
}

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
    json = json_object_new_object();
    json_object_object_add(json, "device", json_object_new_string("Hi-MD"));
    json_object_object_add(json, "title", json_object_new_string("Unknown"));
    int i;
    int recordedTime = 0;
    json_object* tracks = json_object_new_array();
    struct himderrinfo status;
    for(i = HIMD_FIRST_TRACK;i <= HIMD_LAST_TRACK;i++)
    {
        struct trackinfo t;
        if(himd_get_track_info(himd, i, &t, NULL)  >= 0)
        {
            // TODO: malloc name in case of long title
            char *title, *artist, *album, *codec, name[255], time[12];
            json_object* track = json_object_new_object();
            title = get_locale_str(himd, t.title);
            artist = get_locale_str(himd, t.artist);
            album = get_locale_str(himd, t.album);
            sprintf(name,"%s - %s",artist, title);
            recordedTime += t.seconds;
            //TODO handle track times in hours
            sprintf(time, "%02d:%02d.00", t.seconds/60, t.seconds % 60);
            codec = himd_get_codec_name(&t);
            if (verbose < 2) {
              printf("%4d: %d:%02d %s %s:%s (%s %d)%s\n",
                      i, t.seconds/60, t.seconds % 60, codec,
                      artist ? artist : "Unknown artist",
                      title ? title : "Unknown title",
                      album ? album : "Unknown album", t.trackinalbum,
                      himd_track_uploadable(himd, &t) ? " [uploadable]":"");
            }
            json_object_object_add(track, "no",         json_object_new_int(i-1));
            json_object_object_add(track, "protect",    json_object_new_string(himd_track_uploadable(himd, &t) ? "UnPROT":"TrPROT"));
            if (!strcmp(codec, "MPEG")) {
              json_object_object_add(track, "bitrate",    json_object_new_string("MP3"));
            } else {
              json_object_object_add(track, "bitrate",    json_object_new_string(codec));
            }
            json_object_object_add(track, "time",       json_object_new_string(time));
            json_object_object_add(track, "name",       json_object_new_string(name));
            json_object_array_add(tracks, track);
            g_free(title);
            g_free(artist);
            g_free(album);
            if(verbose == 1)
            {
                char rtime[30],stime[30],etime[30];
                struct fraginfo f;
                int fnum = t.firstfrag;
                int blocks;

                if((blocks = himd_track_blocks(himd, &t, &status)) < 0)
                    fprintf(stderr, "Can't get block count for Track %d: %s\n", i, status.statusmsg);
                else
                    printf("     %5d Blocks of 16 KB each,  %dkbps\n",
                                blocks, sony_codecinfo_kbps(&t.codec_info));

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
                printf("     Content ID: %s\n", hexdump(t.contentid, 20));
                printf("     Key: %s (EKB %08x); MAC: %s\n", hexdump(t.key, 8), t.ekbnum, hexdump(t.mac, 8));
                if(t.recordingtime.tm_mon != -1)
                    strftime(rtime,sizeof rtime, "%x %X", &t.recordingtime);
                else
                    strcpy(rtime, "?");
                if(t.licensestarttime.tm_mon != -1)
                    strftime(stime,sizeof stime, "%x %X", &t.licensestarttime);
                else
                    strcpy(stime, "any time");
                if(t.licenseendtime.tm_mon != -1)
                    strftime(etime,sizeof etime, "%x %X", &t.licenseendtime);
                else
                    strcpy(etime, "any time");
                printf("     Recorded: %s, licensed: %s-%s\n", rtime, stime, etime);
            }
        }
    }
    char time[16];
    sprintf(time, "%02d:%02d:%02d.00", recordedTime/3600, (recordedTime % 3600)/60, recordedTime % 60);
    json_object_object_add(json, "recordedTime", json_object_new_string(time));
    json_object_object_add(json, "totalTime", json_object_new_string(time));
    json_object_object_add(json, "tracks", tracks);
    if (verbose == 2) {
      printf("%s\n", json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY));
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
    unsigned char fragkey[8];
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
    while(himd_blockstream_read(&str, block, &firstframe, &lastframe, fragkey, &status) >= 0)
    {
        if(fwrite(block,16384,1,strdumpf) != 1)
        {
            perror("writing dumped stream");
            goto clean;
        }
        printf("%d: %u..%u %s\n",
                blocknum++,firstframe,lastframe,hexdump(fragkey,8));
    }
    if(status.status != HIMD_STATUS_AUDIO_EOF)
        fprintf(stderr,"Error reading MP3 data: %s\n", status.statusmsg);
clean:
    fclose(strdumpf);
    himd_blockstream_close(&str);
}

void himd_dumpmp3(struct himd * himd, int trknum, char * filepath)
{
    struct himd_mp3stream str;
    struct himderrinfo status;
    FILE * strdumpf;
    unsigned int len;
    const unsigned char * data;
    strdumpf = fopen(filepath,"wb");
    if(!strdumpf)
    {
        perror("Opening filepath");
        return;
    }
    if(himd_mp3stream_open(himd, trknum, &str, &status) < 0)
    {
        fprintf(stderr, "Error opening track %d: %s\n", trknum, status.statusmsg);
        return;
    }
    while(himd_mp3stream_read_block(&str, &data, &len, NULL, &status) >= 0)
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
    make_ea3_format_header(header, &trkinfo->codec_info);

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
void himd_dumpnonmp3(struct himd * himd, int trknum, char * filepath)
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

    if(!sony_codecinfo_is_lpcm(&trkinfo.codec_info))
        filename = "stream.oma";

    if (filepath != NULL)
    strdumpf = fopen(filepath,"wb");
    else
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
    if(!sony_codecinfo_is_lpcm(&trkinfo.codec_info) &&
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

void himd_dumpholes(struct himd * h)
{
    int i;
    struct himd_holelist holes;
    struct himderrinfo status;
    if(himd_find_holes(h, &holes, &status) < 0)
    {
        fprintf(stderr, "Collecting holes: %s\n", status.statusmsg);
        return;
    }
    for(i = 0; i < holes.holecnt;i++)
        printf("%d: %05u-%05u\n", i, holes.holes[i].firstblock, holes.holes[i].lastblock);
}

int main(int argc, char ** argv)
{
    int idx;
    struct himd h;
    struct himderrinfo status;
    setlocale(LC_ALL,"");

    if (argc == 2 && (strcmp (argv[1], "help") == 0)) {
      usage(argv[0]);
      return 0;
    }
    else if (argc > 1 && strcmp("usbids", argv[1]) == 0) {
        json_object *json = json_object_new_array();

        const struct minidisc_usb_device_info *cur = minidisc_usb_device_info_first();
        while (cur != NULL) {
            if (cur->device_type == MINIDISC_USB_DEVICE_TYPE_HIMD) {
                json_object *dev = json_object_new_object();
                json_object_object_add(dev, "vendor_id",  json_object_new_int(cur->vendor_id));
                json_object_object_add(dev, "product_id", json_object_new_int(cur->product_id));
                json_object_object_add(dev, "name",       json_object_new_string(cur->name));
                json_object_array_add(json, dev);
            }

            cur = minidisc_usb_device_info_next(cur);
        }

        printf("%s\n", json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY));
        return 0;
    }

    if (argc < 2) {
      printf("Please specify HiMD path and command to be sent. Use \"%s help\" to display a help.\n", argv[0]);
      return 0;
    }

    if(himd_open(&h,argv[1], &status) < 0)
    {
        puts(status.statusmsg);
        return 1;
    }
    if(argc == 2 || strcmp(argv[2],"tracks") == 0) {
      if (argc <= 3)
        himd_trackdump(&h, 0);
      else {
        if (strcmp(argv[3],"verbose") == 0)
          himd_trackdump(&h, 1);
        else if (strcmp(argv[3],"json") == 0)
          himd_trackdump(&h, 2);
        else
          printf("ERROR: Unknown argument - %s\n", argv[3]);
      }
    } else if(strcmp(argv[2],"strings") == 0)
        himd_stringdump(&h);
    else if(strcmp(argv[2],"discid") == 0)
        himd_dumpdiscid(&h);
    else if(strcmp(argv[2],"holes") == 0)
        himd_dumpholes(&h);
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
        if (argc > 4)
          himd_dumpmp3(&h, idx, argv[4]);
        else
          himd_dumpmp3(&h, idx, "stream.mp3");
    }
    else if(strcmp(argv[2],"dumpnonmp3") == 0 && argc > 3)
    {
        idx = 1;
        sscanf(argv[3], "%d", &idx);
        if (argc > 4) {
          himd_dumpnonmp3(&h, idx, argv[4]);
        } else {
          himd_dumpnonmp3(&h, idx, NULL);
        }
    }
    else if(strcmp(argv[2],"writemp3") == 0 && argc > 3)
    {
#ifdef CONFIG_WITH_MAD
  himd_writemp3(&h, argv[3]);
#else
  fputs("Compiled without libmad - no MP3 download support\n", stderr);
#endif
    }

    himd_close(&h);
    return 0;
}
