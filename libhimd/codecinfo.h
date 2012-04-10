#ifndef ATRAC_H
#define ATRAC_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CODEC_ATRAC3 0x00
#define CODEC_ATRAC3PLUS_OR_MPEG 0x01
#define CODEC_LPCM 0x80

#define SONY_VIRTUAL_LPCM_FRAMESIZE 64
#define SONY_ATRAC3_SAMPLES_PER_FRAME 1024
#define SONY_ATRAC3P_SAMPLES_PER_FRAME 2048

#define TRACK_IS_MPEG 0
struct sony_codecinfo {
    unsigned char codec_id;
    unsigned char codecinfo[5];
};

unsigned int sony_codecinfo_bytesperframe(const struct sony_codecinfo *ci);
unsigned int sony_codecinfo_samplesperframe(const struct sony_codecinfo *ci);
unsigned long sony_codecinfo_samplerate(const struct sony_codecinfo *ci);
unsigned int sony_codecinfo_kbps(const struct sony_codecinfo *ci);
unsigned int sony_codecinfo_seconds(const struct sony_codecinfo *ci, unsigned int frames);
const char * sony_codecinfo_codecname(const struct sony_codecinfo *ci);
#define sony_codecinfo_is_lpcm(ci) ((ci)->codec_id == CODEC_LPCM)
#define sony_codecinfo_is_at3(ci)  ((ci)->codec_id == CODEC_ATRAC3)
#define sony_codecinfo_is_mpeg(ci) ((ci)->codec_id == CODEC_ATRAC3PLUS_OR_MPEG && \
                                    ((ci)->codecinfo[0] & 3) == 3)
#define sony_codecinfo_is_at3p(ci) ((ci)->codec_id == CODEC_ATRAC3PLUS_OR_MPEG && \
                                    ((ci)->codecinfo[0] & 3) != 3)

#ifdef __cplusplus
}
#endif

#endif
