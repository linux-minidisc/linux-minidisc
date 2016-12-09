
/**
 * wavfilewriter: Helper class to write .wav file headers
 * Copyright (C) 2016 Thomas Perl <m@thp.io>
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
 **/

#include "wavefilewriter.h"

#include <QIODevice>
#include <QtEndian>
#include <QDebug>

/**
 * Based on the file format description at http://soundfile.sapp.org/doc/WaveFormat/
 **/

struct RIFFHeader {
    char riff[4]; // "RIFF"
    uint32_t total_size;
    char format[4]; // "WAVE"
};

struct ChunkHeader {
    char id[4]; // "fmt " for format chunk, "data" for samples
    uint32_t size;
};

struct FormatChunk {
    int16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t bytesPerSecond;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};

WaveFileWriter::WaveFileWriter()
    : output()
    , updateSize(false)
{
}

WaveFileWriter::~WaveFileWriter()
{
    close();
}

bool
WaveFileWriter::open(const QString &filename, int sampleRate, int sampleSize, int channels)
{
    output.close();
    output.setFileName(filename);
    output.open(QIODevice::WriteOnly);

    RIFFHeader hdr;
    ChunkHeader chk;
    FormatChunk fmt;

    // RIFF header
    memcpy(hdr.riff, "RIFF", 4);
    hdr.total_size = 0; // To be filled later
    memcpy(hdr.format, "WAVE", 4);
    if ((size_t)output.write((char *)&hdr, sizeof(hdr)) != sizeof(hdr)) {
        return false;
    }

    // Format chunk header
    memcpy(chk.id, "fmt ", 4);
    chk.size = qToLittleEndian(uint32_t(sizeof(fmt)));
    if ((size_t)output.write((char *)&chk, sizeof(chk)) != sizeof(chk)) {
        return false;
    }

    // Format chunk
    fmt.audioFormat = qToLittleEndian(int16_t(1)); // PCM
    fmt.numChannels = qToLittleEndian(uint32_t(channels));
    fmt.sampleRate = qToLittleEndian(uint32_t(sampleRate));
    fmt.bytesPerSecond = qToLittleEndian(uint32_t(channels * (sampleSize / 8) * sampleRate));
    fmt.blockAlign = qToLittleEndian(uint32_t(channels * (sampleSize / 8)));
    fmt.bitsPerSample = qToLittleEndian(uint32_t(sampleSize));
    if ((size_t)output.write((char *)&fmt, sizeof(fmt)) != sizeof(fmt)) {
        return false;
    }

    // Data chunk header
    memcpy(chk.id, "data", 4);
    chk.size = 0; // To be filled later
    if ((size_t)output.write((char *)&chk, sizeof(chk)) != sizeof(chk)) {
        return false;
    }

    updateSize = true;
    return true;
}

bool
WaveFileWriter::write_signed_big_endian(const int16_t *data, size_t samples)
{
    int16_t tmp[samples];
    for (size_t i=0; i<samples; i++) {
        tmp[i] = qToLittleEndian(qFromBigEndian(data[i]));
    }
    return ((size_t)output.write(reinterpret_cast<const char *>(tmp), sizeof(tmp)) == sizeof(tmp));
}

static bool
updateU32LESizeField(QFile &file, size_t offset, uint32_t new_value)
{
    if (!file.seek(offset)) {
        qWarning() << "Could not seek to update field";
        return false;
    }

    // The "remaining" size is calculated from after the value (subtract offset + value size)
    new_value = qToLittleEndian(uint32_t(new_value - (offset + sizeof(new_value))));
    if ((size_t)file.write((char *)&new_value, sizeof(new_value)) != sizeof(new_value)) {
        qWarning() << "Could not update field in file";
        return false;
    }

    return true;
}

void
WaveFileWriter::close()
{
    // File size in the RIFF header
    size_t riff_header_offset = offsetof(RIFFHeader, total_size);
    // Chunk size in the "data" chunk
    size_t data_header_offset = sizeof(RIFFHeader) + sizeof(ChunkHeader) + sizeof(FormatChunk) + offsetof(ChunkHeader, size);

    if (updateSize &&
            updateU32LESizeField(output, riff_header_offset, output.size()) &&
            updateU32LESizeField(output, data_header_offset, output.size())) {
        updateSize = false;
    }

    output.close();
}
