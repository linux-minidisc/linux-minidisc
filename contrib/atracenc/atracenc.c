/*
 * atracenc.exe: Command-line utility for WAV->LP2/LP4 conversion
 * Copyright (C) 2022 Thomas Perl <m@thp.io>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


/**
 * HOW TO USE THIS TOOL
 *
 * This requires the "atrac3.acm" file and just uses the normal Windows ACM API
 * for loading the codec and feeding it the WAV file contents. The output file
 * is an ATRAC3 LP2/LP4-encoded WAV file ready for use with "netmdcli send".
 *
 * size(atrac3.acm) = 95292
 * sha1sum(atrac3.acm) = 297fd908e66decc8e266b5969194bdd8c58d3325
 *
 * Tested only under Linux with Wine, but assumed working on Windows.
 * Compile with the i686 GCC from mingw-w64 (i686-w64-mingw32-gcc).
 *
 * The "atrac3.acm" is available in multiple places online, e.g:
 * https://www.rarewares.org/rrw/sonicstage.php
 */


#include <stdio.h>
#include <windows.h>
#include <mmreg.h>
#include <msacm.h>
#include <stdint.h>

struct WaveFormatHeaderAtrac3 {
    WAVEFORMATEX wfx;

    /* The "extradata" is documented/used in:
     *  - netmd_write_wav_header() (libnetmd/secure.c)
     *  - atrac3_decode_init() (libavcodec/atrac3.c)
     *
     * 2 bytes little-endian: unknown value, always 1
     * 4 bytes little-endian: "samples per channel"? ("bytes per frame"?)
     *                        libnetmd says this should be 96 for LP4, 192 for LP2?
     *                        atrac3.acm puts the 4 bytes there: 0x00, 0x10, 0x00, 0x00
     * 2 bytes little-endian: coding mode (joint stereo = 0x0001, stereo = 0x0000)
     * 2 bytes little-endian: dupe of coding mode
     * 2 bytes little-endian: unknown value, always 1
     * 2 bytes little-endian: unknown value, always 0
     */
    uint8_t extradata[14];
};

static const struct WaveFormatHeaderAtrac3
WFX_ATRAC3_66KBPS = {
    .wfx = {
        .wFormatTag = 0x00000270,
        .nChannels = 2,
        .nSamplesPerSec = 44100,
        .nAvgBytesPerSec = 8268,
        .nBlockAlign = 192,
        .wBitsPerSample = 0,
        .cbSize = 14,
    },
    .extradata = { 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00 },
};

static const struct WaveFormatHeaderAtrac3
WFX_ATRAC3_105KBPS = {
    .wfx = {
        .wFormatTag = 0x00000270,
        .nChannels = 2,
        .nSamplesPerSec = 44100,
        .nAvgBytesPerSec = 13092,
        .nBlockAlign = 304,
        .wBitsPerSample = 0,
        .cbSize = 14,
    },
    .extradata = { 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 },
};

static const struct WaveFormatHeaderAtrac3
WFX_ATRAC3_132KBPS = {
    .wfx = {
        .wFormatTag = 0x00000270,
        .nChannels = 2,
        .nSamplesPerSec = 44100,
        .nAvgBytesPerSec = 16537,
        .nBlockAlign = 384,
        .wBitsPerSample = 0,
        .cbSize = 14,
    },
    .extradata = { 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 },
};

struct EncodingMode {
    const char *mode;
    const struct WaveFormatHeaderAtrac3 *hdr;
};

static const struct EncodingMode
MDLP_MODES[] = {
    { "lp2", &WFX_ATRAC3_132KBPS },
    { "105", &WFX_ATRAC3_105KBPS },
    { "lp4", &WFX_ATRAC3_66KBPS },
    { NULL, NULL },
};


struct RIFFHeader {
    char chunk_id[4];
    uint32_t chunk_size;
    char format[4];
};

static void
riff_header_set(struct RIFFHeader *hdr, const char *chunk_id, uint32_t chunk_size, const char *format)
{
    memcpy(hdr->chunk_id, chunk_id, 4);
    hdr->chunk_size = chunk_size;
    memcpy(hdr->format, format, 4);
}

struct ChunkHeader {
    char subchunk_id[4];
    uint32_t subchunk_size;
};

static void
chunk_header_set(struct ChunkHeader *hdr, const char *subchunk_id, uint32_t subchunk_size)
{
    memcpy(hdr->subchunk_id, subchunk_id, 4);
    hdr->subchunk_size = subchunk_size;
}


static void
usage(const char *progname)
{
    printf("Usage: %s <lp2|105|lp4> <infile.wav> <outfile.wav>\n", progname);
}

int
main(int argc, char *argv[])
{
    if (argc != 4) {
        usage(argv[0]);
        return 1;
    }

    const char *mode_name = argv[1];
    const char *src_filename = argv[2];
    const char *dst_filename = argv[3];

    const struct EncodingMode *mode = NULL;

    const struct EncodingMode *cur = MDLP_MODES;
    while (cur->mode != NULL) {
        if (strcmp(cur->mode, mode_name) == 0) {
            mode = cur;
            break;
        }
        ++cur;
    }

    if (mode == NULL) {
        usage(argv[0]);
        return 1;
    }

    static const WAVEFORMATEX
    WAV_FORMAT_CDDA = {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nChannels = 2,
        .nSamplesPerSec = 44100,
        .nAvgBytesPerSec = 44100 * 4,
        .nBlockAlign = 4,
        .wBitsPerSample = 16,
        .cbSize = 0,
    };

    // Format of the WAV file
    WAVEFORMATEX wfxSrc;
    memset(&wfxSrc, 0, sizeof(wfxSrc));

    // Data
    char *wavData = NULL;
    size_t wavSize = 0;

    FILE *fp = fopen(src_filename, "rb");
    if (fp == NULL) {
        printf("Cannot open %s for reading\n", src_filename);
        return 1;
    }

    printf("Parsing '%s'\n", src_filename);

    struct RIFFHeader wavhdr;
    size_t rres = fread(&wavhdr, 1, sizeof(wavhdr), fp);
    if (rres != sizeof(wavhdr)) {
        printf("Could not read RIFF WAV header\n");
        return 1;
    }

    if (memcmp(wavhdr.chunk_id, "RIFF", 4) != 0 || memcmp(wavhdr.format, "WAVE", 4) != 0) {
        printf("Not a valid WAV file: %s\n", src_filename);
        return 1;
    }

    while (!feof(fp)) {
        struct ChunkHeader chkhdr;
        rres = fread(&chkhdr, 1, sizeof(chkhdr), fp);

        if (rres == 0 && feof(fp)) {
            break;
        }

        if (rres != sizeof(chkhdr)) {
            printf("Could not read RIFF chunk header (rres=%zu)\n", rres);
            return 1;
        }

        if (memcmp(chkhdr.subchunk_id, "fmt ", 4) == 0) {
            printf("Format chunk, %u bytes\n", chkhdr.subchunk_size);
            if (chkhdr.subchunk_size != 16) {
                printf("Unexpected format chunk size %d (expected 16)\n", chkhdr.subchunk_size);
                return 1;
            }

            rres = fread(&wfxSrc, 1, chkhdr.subchunk_size, fp);
            if (rres != chkhdr.subchunk_size) {
                printf("Failed to read format chunk data\n");
                return 1;
            }

            if (memcmp(&wfxSrc, &WAV_FORMAT_CDDA, sizeof(WAV_FORMAT_CDDA)) != 0) {
                printf("Invalid WAV format:\n");
                printf(" wFormatTag = 0x%08x (expected 0x%08x)\n", wfxSrc.wFormatTag, WAV_FORMAT_CDDA.wFormatTag);
                printf(" nChannels = %d (expected %d)\n", wfxSrc.nChannels, WAV_FORMAT_CDDA.nChannels);
                printf(" nSamplesPerSec = %d (expected %d)\n", wfxSrc.nSamplesPerSec, WAV_FORMAT_CDDA.nSamplesPerSec);
                printf(" nAvgBytesPerSec = %d (expected %d)\n", wfxSrc.nAvgBytesPerSec, WAV_FORMAT_CDDA.nAvgBytesPerSec);
                printf(" nBlockAlign = %d (expected %d)\n", wfxSrc.nBlockAlign, WAV_FORMAT_CDDA.nBlockAlign);
                printf(" wBitsPerSample = %d (expected %d)\n", wfxSrc.wBitsPerSample, WAV_FORMAT_CDDA.wBitsPerSample);
                printf(" cbSize = %d (expected %d)\n", wfxSrc.cbSize, WAV_FORMAT_CDDA.cbSize);
                return 1;
            }
        } else if (memcmp(chkhdr.subchunk_id, "data", 4) == 0) {
            printf("Data chunk, %u bytes\n", chkhdr.subchunk_size);

            wavData = malloc(chkhdr.subchunk_size);
            wavSize = chkhdr.subchunk_size;

            rres = fread(wavData, 1, wavSize, fp);
            if (rres != wavSize) {
                printf("Failed to read sample data\n");
                return 1;
            }
        } else {
            char str[5];
            memcpy(str, chkhdr.subchunk_id, 4);
            str[4] = '\0';
            printf("Unknown chunk '%s', skipping over %u bytes\n", str, chkhdr.subchunk_size);
            if (fseek(fp, chkhdr.subchunk_size, SEEK_CUR) != 0) {
                printf("Could not seek over unknown chunk\n");
                return 1;
            }
        }
    }

    fclose(fp);

    printf("WAV file loaded, %zu samples bytes\n", wavSize);

    HINSTANCE atrac3_acm = LoadLibrary("atrac3.acm");
    if (atrac3_acm == NULL) {
        printf("Could not load 'atrac3.acm', it's available from here:\n");
        printf("  https://www.rarewares.org/rrw/sonicstage.php\n");
        return 1;
    }

    printf("atrac3.acm loaded\n");

    FARPROC driver_proc = GetProcAddress(atrac3_acm, "DriverProc");
    if (driver_proc == NULL) {
        printf("Could not find 'DriverProc' in 'atrac3.acm'\n");
        return 1;
    }

    printf("DriverProc found\n");

    HACMDRIVERID adid;
    MMRESULT res = acmDriverAdd(&adid, atrac3_acm, (LPARAM)driver_proc, 0, ACM_DRIVERADDF_FUNCTION);
    printf("acmDriverAdd() = %d\n", res);
    if (res != 0) {
        printf("Could not add ATRAC3 driver to ACM\n");
        return 1;
    }

    HACMDRIVER had;
    res = acmDriverOpen(&had, adid, 0);
    printf("acmDriverOpen() = %d\n", res);
    if (res != 0) {
        printf("Could not open ATRAC3 driver in ACM\n");
        return 1;
    }

    HACMSTREAM has;

    struct WaveFormatHeaderAtrac3 wfha = *(mode->hdr);

    res = acmStreamOpen(&has, had, &wfxSrc, &wfha.wfx, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME);
    printf("acmStreamOpen() = %d\n", res);
    if (res != 0) {
        printf("Could not open ACM conversion stream (WAV->ATRAC3)\n");
        return 1;
    }

    // PCM WAV = 1411.2 kbps (44100 Hz * sizeof(uint16_t) * 2 (channels) * 8 (bits) / 1000)
    // LP2 = 132 kbps "usable", but really 146 kbps (292 kbps / 2) with "ATRAC1 silence framing"
    // 105 = 105 kbps (not really a MD format, but can presumably be used over-the-wire,
    //                 it might be useful for using the better-quality LP4 encoder of the device?)
    // LP4 = 66 kbps "usable", but really 73 kbps (292 kbps / 4) with "ATRAC1 silence framing"
    //
    // The encoded data returned seems to be the "usable" data, "ATRAC1 silence framing" is probably
    // only added on MD media, but not in encoded files sent over the wire protocol.
    //
    // To always fit the encoded data, let's assume LP2 uses ~ 10 % the storage space of PCM WAV,
    // add 1 MiB for some wiggle room (pessimistic, but better overestimate than underestimate)
    size_t outSize = 1024 * 1024 + wavSize / 10;
    char *outData = malloc(outSize);

    ACMSTREAMHEADER hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.cbStruct = sizeof(ACMSTREAMHEADER);
    hdr.pbSrc = wavData;
    hdr.cbSrcLength = wavSize;
    hdr.cbSrcLengthUsed = wavSize;
    hdr.pbDst = outData;
    hdr.cbDstLength = outSize;

    res = acmStreamPrepareHeader(has, &hdr, 0);
    printf("acmStreamPrepareHeader() = %d\n", res);
    if (res != 0) {
        printf("Could not prepare ACM stream header\n");
        return 1;
    }

    printf("Starting conversion...\n");
    while (1) {
        res = acmStreamConvert(has, &hdr, ACM_STREAMCONVERTF_START | ACM_STREAMCONVERTF_END);
        printf("acmStreamConvert() = %d\n", res);
        if (res != 0) {
            printf("Error in stream conversion\n");
            return 1;
        }

        printf("cbSrcLengthUsed = %d, cbDstLength = %d, cbDstLengthUsed = %d\n",
                hdr.cbSrcLengthUsed, hdr.cbDstLength, hdr.cbDstLengthUsed);

        if (hdr.cbSrcLengthUsed == hdr.cbSrcLength) {
            printf("Conversion done.\n");
            break;
        } else {
            printf("Unhandled conversion result (partial conversion?)\n");
            return 1;
        }
    }

    res = acmStreamUnprepareHeader(has, &hdr, 0);
    printf("acmStreamUnrepareHeader() = %d\n", res);
    if (res != 0) {
        printf("Error in unprepare header\n");
        return 1;
    }

    res = acmStreamClose(has, 0);
    printf("acmStreamClose() = %d\n", res);
    if (res != 0) {
        printf("Erorr in closing of ACM stream\n");
        return 1;
    }

    fp = fopen(dst_filename, "wb");
    if (fp == NULL) {
        printf("Cannot open %s for writing\n", dst_filename);
        return 1;
    }

    struct RIFFHeader wh;
    riff_header_set(&wh, "RIFF", 4 + (8 + sizeof(struct WaveFormatHeaderAtrac3)) + (8 + hdr.cbDstLengthUsed), "WAVE");
    if (fwrite(&wh, sizeof(wh), 1, fp) != 1) {
        printf("Could not write RIFF header\n");
        return 1;
    }

    struct ChunkHeader fh;
    chunk_header_set(&fh, "fmt ", sizeof(struct WaveFormatHeaderAtrac3));
    if (fwrite(&fh, sizeof(fh), 1, fp) != 1) {
        printf("Could not write format chunk header\n");
        return 1;
    }

    if (fwrite(&wfha, sizeof(wfha), 1, fp) != 1) {
        printf("Could not write format chunk\n");
        return 1;
    }

    struct ChunkHeader dh;
    chunk_header_set(&dh, "data", hdr.cbDstLengthUsed);
    if (fwrite(&dh, sizeof(dh), 1, fp) != 1) {
        printf("Could not write data chunk header\n");
        return 1;
    }

    if (fwrite(hdr.pbDst, hdr.cbDstLengthUsed, 1, fp) != 1) {
        printf("Could not write data chunk\n");
        return 1;
    }

    fclose(fp);

    printf("Encoded:\n    %s (%zu bytes) [pcm]\n -> %s (%zu bytes) [atrac3 %s]\n",
            src_filename, wavSize,
            dst_filename, hdr.cbDstLengthUsed,
            mode->mode);

    free(wavData);
    free(outData);

    return 0;
}
