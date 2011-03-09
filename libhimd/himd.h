#ifndef INCLUDED_LIBHIMD_HIMD_H
#define INCLUDED_LIBHIMD_HIMD_H

#include <time.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CODEC_ATRAC3 0x00
#define CODEC_ATRAC3PLUS_OR_MPEG 0x01
#define CODEC_LPCM 0x80

#define HIMD_ENCODING_LATIN1 5
#define HIMD_ENCODING_UTF16BE 0x84
#define HIMD_ENCODING_SHIFT_JIS 0x90

#define STRING_TYPE_CONTINUATION 1
#define STRING_TYPE_TITLE 8
#define STRING_TYPE_ARTIST 9
#define STRING_TYPE_ALBUM 10
#define STRING_TYPE_GROUP 12 /*reportedly disk/group name */

#define HIMD_FIRST_TRACK 1
#define HIMD_LAST_TRACK 2047

#define HIMD_FIRST_FRAGMENT 1
#define HIMD_LAST_FRAGMENT 4095

#define HIMD_FIRST_STRING 1
#define HIMD_LAST_STRING 4095

#define HIMD_LPCM_FRAMESIZE 64
#define HIMD_ATRAC3_SAMPLES_PER_FRAME 1024
#define HIMD_ATRAC3P_SAMPLES_PER_FRAME 2048

#define HIMD_TIFFILE_SIZE 327680
#define HIMD_AUDIO_SIZE 0x3FC0
#define HIMD_BLOCKINFO_SIZE 0x4000

enum himdstatus { HIMD_OK,
                  HIMD_STATUS_AUDIO_EOF,
                  HIMD_ERROR_DISABLED_FEATURE,
                  HIMD_ERROR_CANT_OPEN_MCLIST,
                  HIMD_ERROR_CANT_READ_MCLIST,
                  HIMD_ERROR_CANT_READ_TIF,
                  HIMD_ERROR_WRONG_TIF_SIZE,
                  HIMD_ERROR_WRONG_TIF_MAGIC,
                  HIMD_ERROR_CANT_ACCESS_HMDHIFI,
                  HIMD_ERROR_NO_TRACK_INDEX,
                  HIMD_ERROR_CANT_OPEN_TRACK_INDEX,
                  HIMD_ERROR_CANT_OPEN_AUDIO,
                  HIMD_ERROR_CANT_SEEK_AUDIO,
                  HIMD_ERROR_CANT_READ_AUDIO,
                  HIMD_ERROR_NO_SUCH_TRACK,
                  HIMD_ERROR_FRAGMENT_CHAIN_BROKEN,
                  HIMD_ERROR_STRING_CHAIN_BROKEN,
                  HIMD_ERROR_STRING_ENCODING_ERROR,
                  HIMD_ERROR_NOT_STRING_HEAD,
                  HIMD_ERROR_UNKNOWN_ENCODING,
                  HIMD_ERROR_BAD_FRAME_NUMBERS,
                  HIMD_ERROR_BAD_AUDIO_CODEC,
                  HIMD_ERROR_BAD_DATA_FORMAT,
                  HIMD_ERROR_UNSUPPORTED_ENCRYPTION,
                  HIMD_ERROR_ENCRYPTION_FAILURE,
                  HIMD_ERROR_OUT_OF_MEMORY };

/* a track on the HiMD */
struct trackinfo {
    int title, artist, album;
    int trackinalbum;
    unsigned char codec_id;
    unsigned char codecinfo[5];
    int firstfrag;	/* index into parts table */
    int tracknum;	/* always equal to own index in used tracks? */
    int seconds;
    unsigned char key[8];
    unsigned char mac[8];
    unsigned char contentid[20];
    int ekbnum;
    struct tm recordingtime, starttime, endtime;
};

/* a fragment in the audio file */
struct fraginfo {
    unsigned char key[8];
    unsigned int firstblock;
    unsigned int lastblock;
    unsigned int firstframe;
    unsigned int lastframe;
    unsigned int fragtype;
    unsigned int nextfrag;
};


/* a block in the audio file */
struct blockinfo {
  unsigned int type;          	     // "LPCM" or "A3D " or "ATX" or "SPMA"
  short int nframes;
  short int mcode;
  short int lendata;
  short int reserved1;
  unsigned int serial_number;
  unsigned char key[8];
  unsigned char iv[8];
  unsigned char audio_data[0x3FC0];		// obfuscated audio data
  unsigned char backup_key[8];
  unsigned char reserved2[8];
  unsigned int backup_type;
  short int backup_reserved;
  short int backup_mcode;
  int lo32_contentid;
  int backup_serial_number;
};

struct himdstring {
    char data[14];
    unsigned int stringtype : 4;
    unsigned int nextstring : 12;
};

struct himd {
    /* everything below this line is private, i.e. no API stability. */
    char * rootpath;
    unsigned char * tifdata;
    int discid_valid;
    unsigned char discid[16];
    int datanum;
    int need_lowercase;
};

struct himderrinfo {
    enum himdstatus status;
    char statusmsg[128];
};

int himd_open(struct himd * himd, const char * himdroot, struct himderrinfo * status);
void himd_close(struct himd * himd);
char* himd_get_string_raw(struct himd * himd, unsigned int idx, int*type, int* length, struct himderrinfo * status);
char* himd_get_string_utf8(struct himd * himd, unsigned int idx, int*type, struct himderrinfo * status);
int himd_add_string(struct himd * himd, char *string, int type, int length, struct himderrinfo * status);
void himd_free(void * p);
const unsigned char * himd_get_discid(struct himd * himd, struct himderrinfo * status);
FILE * himd_open_file(struct himd * himd, const char * fileid);
int himd_write_tifdata(struct himd * himd, struct himderrinfo * status);
unsigned int himd_track_count(struct himd * himd);
unsigned int himd_get_trackslot(struct himd * himd, int unsigned idx, struct himderrinfo * status);

int himd_get_track_info(struct himd * himd, unsigned int idx, struct trackinfo * track, struct himderrinfo * status);
int himd_get_fragment_info(struct himd * himd, unsigned int idx, struct fraginfo * f, struct himderrinfo * status);
int himd_track_uploadable(struct himd * himd, const struct trackinfo * track);
int himd_track_blocks(struct himd * himd, const struct trackinfo * track, struct himderrinfo * status);

int himd_get_free_trackindex(struct himd * himd);
int himd_add_track_info(struct himd * himd, struct trackinfo * track, struct himderrinfo * status);
int himd_add_fragment_info(struct himd * himd, struct fraginfo * f, struct himderrinfo * status);

const char * himd_get_codec_name(const struct trackinfo * t);
unsigned int himd_trackinfo_framesize(const struct trackinfo * track);
unsigned int himd_trackinfo_framesperblock(const struct trackinfo * track);

typedef unsigned char mp3key[4];
int himd_obtain_mp3key(struct himd * himd, int track, mp3key * key, struct himderrinfo * status);

/* data stream, mdstream.c */

struct himd_blockstream {
    struct himd * himd;
    FILE * atdata;
    struct fraginfo *frags;
    unsigned int curblockno;
    unsigned int curfragno;
    unsigned int fragcount;
    unsigned int blockcount;
    unsigned int frames_per_block;
};

#define TRACK_IS_MPEG 0

int himd_blockstream_open(struct himd * himd, unsigned int firstfrag, unsigned int frames_per_block, struct himd_blockstream * stream, struct himderrinfo * status);
void himd_blockstream_close(struct himd_blockstream * stream);
int himd_blockstream_read(struct himd_blockstream * stream, unsigned char * block,
                            unsigned int * firstframe, unsigned int * lastframe,
                            unsigned char * fragkey, struct himderrinfo * status);


struct himd_writestream {
    struct himd * himd;
    FILE * atdata;
    unsigned int curblockno;
};

int himd_writestream_open(struct himd * himd, struct himd_writestream * stream,  unsigned int * out_first_blockno, unsigned int * out_last_blockno, struct himderrinfo * status);

int himd_writestream_write(struct himd_writestream * stream, struct blockinfo *block, struct himderrinfo * status);
void himd_writestream_close(struct himd_writestream * stream);


struct himd_mp3stream {
    struct himd_blockstream stream;
    unsigned char blockbuf[16384];
    const unsigned char ** frameptrs;
    mp3key key;
    unsigned int curframe;
    unsigned int frames;
};

int himd_mp3stream_open(struct himd * himd, unsigned int trackno, struct himd_mp3stream * stream, struct himderrinfo * status);
int himd_mp3stream_read_frame(struct himd_mp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, struct himderrinfo * status);
int himd_mp3stream_read_block(struct himd_mp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, unsigned int * framecount, struct himderrinfo * status);
void himd_mp3stream_close(struct himd_mp3stream * stream);

#define HIMD_MAX_PCMFRAME_SAMPLES (0x3FC0/4)

struct himd_nonmp3stream {
    struct himd_blockstream stream;
    void * cryptinfo;
    unsigned char blockbuf[16384];
    int framesize;
    const unsigned char * frameptr;
    unsigned int framesleft;
};

int himd_nonmp3stream_open(struct himd * himd, unsigned int trackno, struct himd_nonmp3stream * stream, struct himderrinfo * status);
int himd_nonmp3stream_read_frame(struct himd_nonmp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, struct himderrinfo * status);
int himd_nonmp3stream_read_block(struct himd_nonmp3stream * stream, const unsigned char ** frameout, unsigned int * lenout, unsigned int * framecount, struct himderrinfo * status);
void himd_nonmp3stream_close(struct himd_nonmp3stream * stream);

/* frag.c */
struct himd_hole {
    unsigned short firstblock;
    unsigned short lastblock;
};

struct himd_holelist {
    int holecnt;
    struct himd_hole holes[HIMD_LAST_FRAGMENT - HIMD_FIRST_FRAGMENT + 1];
};

int himd_find_holes(struct himd * himd, struct himd_holelist * holes, struct himderrinfo * status);


#ifdef __cplusplus
}
#endif

#endif
