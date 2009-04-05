
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

enum himdstatus { HIMD_OK,
                  HIMD_ERROR_CANT_READ_TRACKS,
                  HIMD_ERROR_CANT_READ_PARTS,
                  HIMD_ERROR_CANT_READ_STRINGS,
                  HIMD_ERROR_CANT_ACCESS_HMDHIFI,
                  HIMD_ERROR_NO_TRACK_INDEX,
                  HIMD_ERROR_CANT_OPEN_TRACK_INDEX,
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
    int firstpart;	/* index into parts table */
    int tracknum;	/* always equal to own index in used tracks? */
    int seconds;
};

/* a fragment in the audio file */
struct partinfo {
    unsigned int firstblock;
    unsigned int lastblock;
    unsigned int firstframe;
    unsigned int lastframe;
    unsigned int nextpart;
};

struct himdstring {
    char data[14];
    unsigned int stringtype : 4;
    unsigned int nextstring : 12;
};

struct himd {
    char * rootpath;
    char discid[16];
    int datanum;
    struct trackinfo tracks[2048];
    struct partinfo parts[4096];
    struct himdstring strings[4096];
    enum himdstatus status;
    char statusmsg[64];
};

int himd_open(struct himd * himd, const char * himdroot);
struct himd * himd_new(const char * pathname);
char* himd_get_string_raw(struct himd * himd, unsigned int idx, int*type, int* length);
char* himd_get_string_utf8(struct himd * himd, unsigned int idx, int*type);
void himd_free(void * p);
