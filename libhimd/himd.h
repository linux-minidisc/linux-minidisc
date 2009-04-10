#include <stdio.h>

#define CODEC_LOSSY 0x01
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

enum himdstatus { HIMD_OK,
                  HIMD_STATUS_AUDIO_EOF,
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

struct himdstring {
    char data[14];
    unsigned int stringtype : 4;
    unsigned int nextstring : 12;
};

struct himd {
    enum himdstatus status;
    char statusmsg[128];
    /* everything below this line is private, i.e. no API stability. */
    char * rootpath;
    unsigned char * tifdata;
    int discid_valid;
    unsigned char discid[16];
    int datanum;
};

int himd_open(struct himd * himd, const char * himdroot);
void himd_close(struct himd * himd);
char* himd_get_string_raw(struct himd * himd, unsigned int idx, int*type, int* length);
char* himd_get_string_utf8(struct himd * himd, unsigned int idx, int*type);
void himd_free(void * p);
const unsigned char * himd_get_discid(struct himd * himd);
FILE * himd_open_file(struct himd * himd, const char * fileid);

int himd_get_track_info(struct himd * himd, unsigned int idx, struct trackinfo * track);
int himd_get_fragment_info(struct himd * himd, unsigned int idx, struct fraginfo * f);

typedef unsigned char mp3key[4];
int himd_obtain_mp3key(struct himd * himd, int track, mp3key * key);

/* data stream, mdstream.c */

struct himd_blockstream {
    struct himd * himd;
    FILE * atdata;
    struct fraginfo *frags;
    unsigned int curblockno;
    unsigned int curfragno;
    unsigned int fragcount;
    unsigned int blockcount;
    int status;
    char statusmsg[64];
};


int himd_blockstream_open(struct himd * himd, unsigned int firstfrag, struct himd_blockstream * stream);
void himd_blockstream_close(struct himd_blockstream * stream);
int himd_blockstream_read(struct himd_blockstream * stream, unsigned char * block,
                            int * firstframe, int * lastframe);
