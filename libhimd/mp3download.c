/**
 * mp3download.c: Download MP3 files from host to HiMD device
 **/

#include "himd.h"

#include <glib.h>

#ifdef CONFIG_WITH_MAD

#include <mad.h>
#include <id3tag.h>

void block_init(struct blockinfo * b, short int nframes, short int lendata, unsigned int serial_number, unsigned char * cid)
{
    memcpy((char*)&b->type, "SMPA", 4);
    b->nframes       = nframes;
    b->mcode         = 3;
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

int bucket_append(struct abucket * pbucket, guchar * pframe, guint framelen)
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

#define HIMD_MP3_VAR_VERSION 0x40
#define HIMD_MP3_VAR_LAYER   0x20
#define HIMD_MP3_VAR_BITRATE 0x10
#define HIMD_MP3_VAR_SRATE   0x08
#define HIMD_MP3_VAR_CHMODE  0x04
#define HIMD_MP3_VAR_PREEMPH 0x02

gint write_blocks(struct mad_stream *stream, struct himd_writestream *write_stream, mp3key key,
                   mad_timer_t *duration, gint *nblocks, gint *nframes, unsigned char * cid,
                   unsigned char *mp3codecinfo, struct himderrinfo * status)
{
    guchar var_flags = 0x80;
    unsigned mpegvers = 3, mpeglayer = 1, mpegbitrate = 9, mpegsamprate = 0, 
             mpegchmode = 0, mpegpreemph = 0;
    gboolean firsttime = TRUE;
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
        guchar * pframe = (gpointer) stream->this_frame;
	gint framelen = (guint) (stream->next_frame - stream->this_frame);
	/* "b" means "this Block" */
        unsigned bmpegvers, bmpeglayer, bmpegbitrate, bmpegsamprate, 
                 bmpegchmode, bmpegpreemph;
        
        bmpegvers =     (pframe[1] >> 3) & 0x03;
        bmpeglayer =    (pframe[1] >> 1) & 0x03;
        bmpegbitrate =  (pframe[2] >> 4) & 0x0F;
        bmpegsamprate = (pframe[2] >> 2) & 0x03;
        bmpegchmode =   (pframe[3] >> 6) & 0x03;
        bmpegpreemph =  (pframe[3] >> 0) & 0x03;

	mad_timer_add(&mad_timer, header.duration);

	if(firsttime) {
            bmpegvers =     mpegvers;
            bmpeglayer =    mpeglayer;
            bmpegbitrate =  mpegbitrate;
            bmpegsamprate = mpegsamprate;
            bmpegchmode =   mpegchmode;
            bmpegpreemph =  mpegpreemph;
	    firsttime = FALSE;
	} else {
	    if(bmpegvers != mpegvers) {
	        var_flags |= HIMD_MP3_VAR_VERSION;
	        mpegvers = MIN(mpegvers, bmpegvers); /* smaller num -> higher version */
	    }
	    if(bmpeglayer != mpeglayer) {
	        var_flags |= HIMD_MP3_VAR_LAYER;
	        mpeglayer = MIN(mpeglayer, bmpeglayer); /* smaller num -> higher layer */
	    }
	    if(bmpegbitrate != mpegbitrate) {
	        /* TODO: check whether "free-form" streams need special handling */
	        var_flags |= HIMD_MP3_VAR_BITRATE;
	        mpegbitrate = MAX(mpegbitrate, bmpegbitrate);
	    }
	    if(bmpegsamprate != mpegsamprate) {
	        var_flags |= HIMD_MP3_VAR_SRATE;
	        /* "1" is highest (48), "0" is medium (44), "2" is lowest (32) */
	        if(mpegsamprate != 1) {
                    if(bmpegsamprate == 1)
                        mpegsamprate = bmpegsamprate;
                    else
                        mpegsamprate = MIN(mpegsamprate, bmpegsamprate);
                }
	    }
	    if(bmpegchmode != mpegchmode)
	        /* TODO: find out how to choose "maximal" mode */
                var_flags |= HIMD_MP3_VAR_CHMODE;
            if(bmpegpreemph != mpegpreemph)
                /* TODO: find out how to choose "maximal" preemphasis */
                var_flags |= HIMD_MP3_VAR_PREEMPH;
	}

	// Append frames to block
	gint nbytes_added = bucket_append(&bucket, pframe, framelen);
	if(nbytes_added < 0) {
            block_init(&bucket.block, bucket.nframes, bucket.totsize, iblock, cid);

	    // Encrypt block
	    unsigned i=0;
	    for(i=0;i < (bucket.totsize & ~7U); i++)
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

    mp3codecinfo[0] = var_flags;
    mp3codecinfo[1] = (mpegvers << 6) | (mpeglayer << 4) | (mpegbitrate);
    mp3codecinfo[2] = (mpegsamprate << 6) | (mpegchmode << 4) | (mpegpreemph << 2);

    // close write-stream to atdata file
    return iblock;
}

int himd_writemp3(struct himd * h, const char *filepath)
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
    unsigned char mp3codecinfo[3];

    // Generate random content ID
    for(i = 4; i <=19; i++)
        cid[i] = g_random_int_range(0,0xFF);

    // Get track ID3 information
    if(himd_get_songinfo(filepath, &artist, &title, &album, &status) < 0)
        printf("no tags\n");

    // Fallback to filename for title
    if (title == NULL) {
        // Returned allocated string is free'd below
        title = g_path_get_basename(filepath);

        char *dot = strrchr(title, '.');
        if (dot != NULL) {
            *dot = '\0';
        }
    }

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
            return 1;
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
            return 1;
	}

    write_blocks(&stream, &write_stream, key, &duration, &nblocks, &nframes, cid, mp3codecinfo, &status);

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
    track.codec_info.codec_id = CODEC_ATRAC3PLUS_OR_MPEG;
    track.seconds      = duration.seconds;

    track.codec_info.codecinfo[0] = 3;
    track.codec_info.codecinfo[1] = 0;
    track.codec_info.codecinfo[2] = mp3codecinfo[0];
    track.codec_info.codecinfo[3] = mp3codecinfo[1];
    track.codec_info.codecinfo[4] = mp3codecinfo[2];

    memset(&track.mac, 0, 8);
    memcpy(&track.contentid, cid, 20);
    memset(&track.recordingtime,    0, sizeof(struct tm));
    memset(&track.licensestarttime, 0, sizeof(struct tm));
    memset(&track.licenseendtime,   0, sizeof(struct tm));

    /* set DRM stuff correctly for compatibility reasons */
    track.lt = 0x10;
    track.dest = 1;
    track.xcc = 1;
    track.ct = 0;
    track.cc = 0x40;
    track.cn = 0;

    idx_track = himd_add_track_info(h, &track, &status);
    // END: Add track descriptor

    //
    // Update TRACK-INDEX file with track strings, fragment descriptor and track-descriptor.
    //
    himd_write_tifdata(h, &status);
    free(artist); free(album); free(title);

    return 0;
}

#else

int himd_writemp3(struct himd *h, const char *filepath)
{
    (void)h;
    (void)filepath;

    // Not compiled with libmad
    return 2;
}

#endif
