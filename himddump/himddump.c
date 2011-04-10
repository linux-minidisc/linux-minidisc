/*
 *   himddump.c - list contents (tracks, holes), dump tracks and show diskid of a HiMD 
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <locale.h>
#include <string.h>
#include <mad.h>
#include <id3tag.h>
#include <glib/gstdio.h>

#include "himd.h"
#include "sony_oma.h"

void usage(char * cmdname)
{
  printf("Usage: %s <HiMD path> <command>, where <command> is either of:\n\n\
          strings          - dumps all strings found in the tracklist file\n\
          tracks           - lists all tracks on disc\n\
          tracks verbose   - lists details of all tracks on disc\n\
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
                char rtime[30],stime[30],etime[30];
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
                printf("     Content ID: %s\n", hexdump(t.contentid, 20));
                printf("     Key: %s (EKB %08x); MAC: %s\n", hexdump(t.key, 8), t.ekbnum, hexdump(t.mac, 8));
                if(t.recordingtime.tm_mon != -1)
                    strftime(rtime,sizeof rtime, "%x %X", &t.recordingtime);
                else
                    strcpy(rtime, "?");
                if(t.starttime.tm_mon != -1)
                    strftime(stime,sizeof stime, "%x:%X", &t.starttime);
                else
                    strcpy(stime, "?");
                if(t.endtime.tm_mon != -1)
                    strftime(etime,sizeof etime, "%x:%X", &t.endtime);
                else
                    strcpy(etime, "?");
                printf("     Recorded: %s, start: %s, end: %s\n", rtime, stime, etime);
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

/*
 * gets artist, title and album info from an ID3 tag.
 * The output strings are to be free()d.
 */
void get_songinfo(const char *filepath, gchar ** artist, gchar ** title, gchar **album)
{
    struct id3_file * file;
    struct id3_frame const *frame;
    struct id3_tag *tag;
    union id3_field const *field;

    file = id3_file_open(filepath, ID3_FILE_MODE_READONLY);

    tag = id3_file_tag(file);
    if(!tag)
	{
	    printf("no tags\n");
	    id3_file_close(file);
	    return;
	}

    frame = id3_tag_findframe (tag, ID3_FRAME_ARTIST, 0);
    if(frame && (field = &frame->fields[1]) && 
                 id3_field_getnstrings(field) > 0)
        *artist = (gchar*) id3_ucs4_utf8duplicate(id3_field_getstrings(field,0));
    else
        *artist = NULL;

    frame = id3_tag_findframe (tag, ID3_FRAME_TITLE, 0);
    if(frame && (field = &frame->fields[1]) &&
                 id3_field_getnstrings(field) > 0)
        *title = (gchar*) id3_ucs4_utf8duplicate(id3_field_getstrings(field,0));
    else
        *title = NULL;

    frame = id3_tag_findframe (tag, ID3_FRAME_ALBUM, 0);
    if(frame && (field = &frame->fields[1]) &&
                 id3_field_getnstrings(field) > 0)
        *album = (gchar*) id3_ucs4_utf8duplicate(id3_field_getstrings(field,0));
    else
        *album = NULL;

    id3_file_close(file);
}

void block_init(struct blockinfo * b, short int nframes, short int lendata, unsigned int serial_number, unsigned char * cid)
{
    strncpy((char*)&b->type, "SPMA", 4);
    b->nframes       = nframes;
    b->mcode         = 0;
    b->lendata       = lendata;
    b->reserved1     = 0;
    b->serial_number = serial_number;
    memset(&b->key, 0, 8);
    //    print_hex((unsigned char*)&b->key, 8);
    memset(&b->iv, 0, 8);
    memset(&b->backup_key, 0, 8);
    b->backup_type   = b->type;
    memset(&b->reserved2, 0, 8);
    b->backup_reserved      = 0;
    b->backup_mcode         = b->mcode;
    b->lo32_contentid       = cid[16]*16777216+cid[17]*65536+cid[18]*256+cid[19];
    b->backup_serial_number = b->serial_number;
}

void block_printinfo(struct blockinfo * b)
{
    printf("block %d, nframes: %d, lendata: %d\n",
	   b->serial_number, b->nframes, b->lendata);
}

struct abucket
{
    gint totsize;
    gint nframes;
    unsigned char *pbuf_current, *pbuf_end;
    struct blockinfo block;
};

void bucket_init(struct abucket * pbucket)
{
    g_assert(pbucket != NULL);
    memset(&pbucket->block, 0, sizeof(struct blockinfo));

    pbucket->totsize = 0;
    pbucket->nframes = 0;
    pbucket->pbuf_current = &pbucket->block.audio_data[0];
    pbucket->pbuf_end     = &pbucket->block.audio_data[HIMD_AUDIO_SIZE];
}

int bucket_append(struct abucket * pbucket, gchar * pframe, guint framelen)
{
    g_assert(pbucket != NULL);
    g_assert(pframe != NULL);

    gint nbytes_to_add = framelen;

    // Buffer full? or too big frame for buffer?
    if( (pbucket->totsize + nbytes_to_add) >= HIMD_AUDIO_SIZE)
	{
	    if(pbucket->totsize == 0)
		{
		    return 0;
		}
	    return -1;
	}

    g_assert(pbucket->pbuf_current <= pbucket->pbuf_end);

    memcpy(pbucket->pbuf_current, pframe, nbytes_to_add);

    pbucket->pbuf_current += nbytes_to_add;
    pbucket->totsize += nbytes_to_add;
    pbucket->nframes += 1;

    return nbytes_to_add;
}

//
// Input parameters:
//
//  A opened mp3-stream, himd-write-stream, duration structure, (TODO) block-obfuscation-key
//
// Return values:
//
//  Return the number of written blocks and frames
//
// Side-effects:
//
//  Writes audio blocks at the end of the ATDATA container file. Audio blocks contains all frames (TODO: ID3 frames)
//  in a obfuscated form using a 4 byte key.
//
gint write_blocks(struct mad_stream *stream, struct himd_writestream *write_stream, mp3key key,
                   mad_timer_t *duration, gint *nblocks, gint *nframes, unsigned char * cid, struct himderrinfo * status)
{
    struct abucket bucket;
    struct mad_header header;
    mad_timer_t mad_timer;

    gint iblock=0, iframe=0;

    mad_timer_reset(&mad_timer);
    bucket_init(&bucket);

    while(1) {

	if(mad_header_decode(&header, stream) == -1) {
	    if(MAD_RECOVERABLE(stream->error))
		{
		    continue;
		}
	    else {
		break;
	    }
	}
        gchar * pframe = (gpointer) stream->this_frame;
	gint framelen = (guint) (stream->next_frame - stream->this_frame);

	mad_timer_add(&mad_timer, header.duration);

	// Append frames to block
	gint nbytes_added = bucket_append(&bucket, pframe, framelen);
	if(nbytes_added < 0) {
            block_init(&bucket.block, bucket.nframes, bucket.totsize, iblock, cid);

	    // Encrypt block
	    int i=0;
	    for(i=0;i < bucket.totsize; i++)
		bucket.block.audio_data[i] ^= key[i & 3];

	    // Append block to ATDATA file
	    if(himd_writestream_write(write_stream, &bucket.block, status) < 0)
		{
		    fprintf(stderr, "Failed to write block: %d", iblock);
		    perror("write block");
		}

            // remember number of frames in current audio block
            iframe = bucket.nframes;

	    bucket_init(&bucket);

	    // Append the frame to a new block, that not would fit in the previous full block
	    nbytes_added = bucket_append(&bucket, pframe, framelen);
	    if(nbytes_added < 0) {
		exit(1);
	    }

	    iblock += 1;
	    continue;
	}
	else if(nbytes_added == 0) {
            bucket_init(&bucket);
	    continue;
	}
    }

    if( (nblocks != NULL) && (nframes != NULL) && (duration != NULL))
	{
	    *nblocks = iblock;
	    *nframes = iframe;
	    duration->seconds = mad_timer.seconds;
	}

    // close write-stream to atdata file
    return iblock;
}

void himd_writemp3(struct himd  *h, const char *filepath)
{
    struct himderrinfo status;
    gint nblocks=0, nframes=0;
    struct mad_stream stream;
    mad_timer_t duration;
    GMappedFile * mp3file;
    unsigned long mp3size;
    gchar * mp3buffer;
    gchar * artist=NULL, * title=NULL, * album=NULL;
    int i;
    unsigned char cid[20] = {0x02, 0x03, 0x00, 0x00};

    // Generate random content ID
    for(i = 4; i <=19; i++)
        cid[i] = g_random_int_range(0,0xFF);

    // Get track ID3 information
    get_songinfo(filepath, &artist, &title, &album);

    // Load mp3 stream
    mp3file   = g_mapped_file_new(filepath, FALSE, NULL);
    mp3size   = g_mapped_file_get_length(mp3file);
    mp3buffer = g_mapped_file_get_contents(mp3file);

    mad_stream_init(&stream);
    mad_stream_buffer(&stream, (unsigned char*)mp3buffer, mp3size);

    //
    // Get track-key using track-index
    //
    gint idx_track;
    mp3key key;
    idx_track = himd_get_free_trackindex(h);

    if(himd_obtain_mp3key(h, idx_track, &key, &status) < 0)
	{
	    printf("Cannot obtain mp3key\n");
	    exit(1);
	}
    // END: Get track-key

    //
    // Write blocks to ATDATA
    //
    struct himd_writestream write_stream;
    unsigned int first_blockno=0;
    unsigned int last_blockno=0;

    if(himd_writestream_open(h, &write_stream, &first_blockno, &last_blockno, &status) < 0)
	{
	    fprintf(stderr, "Error opening write stream\n");
	    exit(1);
	}

    write_blocks(&stream, &write_stream, key, &duration, &nblocks, &nframes, cid, &status);

    himd_writestream_close(&write_stream);
    // END: Write blocks to ATDATA

    //
    // Calculate blocknumber of the last written block
    //
    last_blockno = first_blockno + nblocks-1;

    //
    // Add fragment descriptor, get back fragment number
    //
    struct fraginfo fragment;
    gint idx_frag;

    fragment.firstblock = first_blockno;
    fragment.lastblock  = last_blockno;
    memset(&fragment.key[0], 0, 8); /* use zero key on mp3 files */
    fragment.firstframe = 0;
    fragment.lastframe  = nframes;
    fragment.fragtype   = 1;
    fragment.nextfrag   = 0;

    idx_frag  = himd_add_fragment_info(h, &fragment, &status);
    // END: Add fragment

    // Add strings for title, album and artist. Retrieve string index numbers.
    gint idx_title=0, idx_album=0, idx_artist=0;

    if(title != NULL) {
	idx_title  = himd_add_string(h, title, STRING_TYPE_TITLE, &status);
	if(idx_title < 0)
	    {
		printf("Failed to add title string\n");
		idx_title = 0;
	    }
    }

    if(album != NULL) {
	idx_album  = himd_add_string(h, album, STRING_TYPE_ALBUM, &status);
	if(idx_album < 0)
	    {
		printf("Failed to add album string\n");
		idx_album = 0;
	    }
    }

    if(artist != NULL) {
	idx_artist = himd_add_string(h, artist, STRING_TYPE_ARTIST, &status);
	if(idx_artist < 0)
	    {
		printf("Failed to add artist string\n");
		idx_artist = 0;
	    }
    }
    // END: Add strings

    //
    // Add track descriptor, get trackno back.
    //
    struct trackinfo track;

    memset(&track.key, 0, 8); /* use zero key on mp3 files */
    track.title  = idx_title;
    track.artist = idx_artist;
    track.album  = idx_album;
    track.firstfrag    = idx_frag;
    track.tracknum     = 1;
    track.ekbnum       = 0;
    track.trackinalbum = 1;
    track.codec_id     = CODEC_ATRAC3PLUS_OR_MPEG;
    track.seconds      = duration.seconds;
    memset(&track.codecinfo, 0, 5);
    track.codecinfo[0] = 3;

    /* file dependent codec information, these values are for my test mp3 file only, */
    /* values fetched from trkidx file by downloading the same mp3 file with SonicStage */
    track.codecinfo[2] = 0xB0; /* mp3, stereo, 128kb/s@44k1Hz */
    track.codecinfo[3] = 0xD9;
    track.codecinfo[4] = 0x10;

    memset(&track.mac, 0, 8);
    memcpy(&track.contentid, cid, 20);
    memset(&track.recordingtime, 0, sizeof(struct tm));
    memset(&track.starttime,     0, sizeof(struct tm));
    memset(&track.endtime,       0, sizeof(struct tm));

    /* set DRM stuff correctly for compatibility reasons */
    track.Lt = 0x10;
    track.Dest = 1;
    track.Xcc = 1;
    track.Cc = 0x40;

    idx_track = himd_add_track_info(h, &track, &status);
    // END: Add track descriptor

    //
    // Update TRACK-INDEX file with track strings, fragment descriptor and track-descriptor.
    //
    himd_write_tifdata(h, &status);
    free(artist); free(album); free(title);
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

    if (argc < 2) {
      printf("Please specify HiMD path and command to be sent. Use \"%s help\" to display a help.\n", argv[0]);
      return 0;
    }

    if(himd_open(&h,argv[1], &status) < 0)
    {
        puts(status.statusmsg);
        return 1;
    }
    if(argc == 2 || strcmp(argv[2],"tracks") == 0)
        himd_trackdump(&h, argc > 3);
    else if(strcmp(argv[2],"strings") == 0)
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
        himd_dumpmp3(&h, idx);
    }
    else if(strcmp(argv[2],"dumpnonmp3") == 0 && argc > 3)
    {
        idx = 1;
        sscanf(argv[3], "%d", &idx);
        himd_dumpnonmp3(&h, idx);
    }
    else if(strcmp(argv[2],"writemp3") == 0 && argc > 3)
    {
	himd_writemp3(&h, argv[3]);
    }

    himd_close(&h);
    return 0;
}
