#
# himd.py: Parsing of Hi-MD track info files (based on libhimd)
# Copyright (c) 2022 Thomas Perl <m@thp.io>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#

import argparse
import binascii
import struct

HIMD_MAGIC_OFFSET = 0x0  # file magic "TIF " + some other bytes (e.g. "01 00 01 25"?)
HIMD_INDEX_OFFSET = 0x100  # until 0x1102 for 2048 tracks (2 + 2 * 2048)
# there is a hole from 0x1102 until 0x8000 that is probably unused? (all zero'd out)
HIMD_TRACK_OFFSET = 0x8000  # until 0x30000 for 2048 tracks (0x50 * 2048)
HIMD_FRAGMENT_OFFSET = 0x30000  # until 0x40000 for 4096 fragments (0x10 * 4096)
HIMD_STRING_OFFSET = 0x40000  # until 0x50000 (=eof) for 4096 strings (0x10 * 4096)
HIMD_TIF_SIZE = 0x50000

SIZEOF_TRACK_CHUNK = 0x50
SIZEOF_FRAGMENT_CHUNK = 0x10
SIZEOF_STRING_CHUNK = 0x10

HIMD_NUM_TRACKS = 2048
HIMD_NUM_FRAGMENTS = 4096
HIMD_NUM_STRINGS = 4096

CODEC_ATRAC3 = 0x00
CODEC_ATRAC3PLUS_OR_MPEG = 0x01
CODEC_LPCM = 0x80

CODECS = {
    CODEC_ATRAC3: 'at3',
    CODEC_ATRAC3PLUS_OR_MPEG: 'at3+/mpeg',
    CODEC_LPCM: 'lpcm',
}

HIMD_ENCODING_LATIN1 = 5
HIMD_ENCODING_UTF16BE = 0x84
HIMD_ENCODING_SHIFT_JIS = 0x90

# If the track's ekbnum is this value, it's uploadable
HIMD_WELL_KNOWN_EKB_NUM = 0x10012

ENCODING_TYPES = {
    HIMD_ENCODING_LATIN1: 'latin-1',
    HIMD_ENCODING_UTF16BE: 'utf-16be',
    HIMD_ENCODING_SHIFT_JIS: 'shift-jis',
}

STRING_TYPE_UNUSED = 0
STRING_TYPE_CONTINUATION = 1
STRING_TYPE_TITLE = 8
STRING_TYPE_ARTIST = 9
STRING_TYPE_ALBUM = 10
STRING_TYPE_GROUP = 12 # reportedly disk/group name

STRING_TYPES = {
    STRING_TYPE_UNUSED: 'unused',
    STRING_TYPE_CONTINUATION: 'continuation',
    STRING_TYPE_TITLE: 'title',
    STRING_TYPE_ARTIST: 'artist',
    STRING_TYPE_ALBUM: 'album',
    STRING_TYPE_GROUP: 'group',
}

parser = argparse.ArgumentParser(description='Hi-MD Parsing Test')
parser.add_argument('filename', metavar='TRKIDX00.HMA', type=str, help='Path to the track index file')
parser.add_argument('--tracks', action='store_true', help='Print track chunks')
parser.add_argument('--fragments', action='store_true', help='Print fragment chunks')
parser.add_argument('--strings', action='store_true', help='Print string chunks')

args = parser.parse_args()

with open(args.filename, 'rb') as fp:
    tifdata = fp.read()

assert len(tifdata) == HIMD_TIF_SIZE
assert tifdata[HIMD_MAGIC_OFFSET:HIMD_MAGIC_OFFSET+4] == b'TIF '

def parse_dostime(dos_date, dos_time):
    # TODO: Time zone handling(?)

    #       |---||----||---|
    #       5432109876543210
    # Bits: HHHHHMMMMMMSSSSS
    #       Hour Min   Sec/2
    seconds = (dos_time & 0b11111) * 2
    minutes = (dos_time >> 5) & 0b111111
    hours = dos_time >> 11
    time = f'{hours:02}:{minutes:02}:{seconds:02}'

    #       |-----||--||---|
    #       5432109876543210
    # Bits: YYYYYYYMMMMDDDDD
    #       Year-80Mon   Day
    day = (dos_date & 0b11111)
    month = (dos_date >> 5) & 0b1111
    year = 1980 + (dos_date >> 9)
    date = f'{year}-{month:02}-{day:02}'

    return (date, time)

(num_tracks,) = struct.unpack('>H', tifdata[HIMD_INDEX_OFFSET:HIMD_INDEX_OFFSET+2])
print(f'Number of tracks: {num_tracks}')

i = 0
while i < num_tracks:
    (track_idx,) = struct.unpack('>H', tifdata[HIMD_INDEX_OFFSET+2+2*i:HIMD_INDEX_OFFSET+2+2*(i+1)])
    print(f'Track {i+1} is in track chunk {track_idx}')
    i += 1

if args.tracks:
    trkoff = HIMD_TRACK_OFFSET
    i = 0
    while trkoff < HIMD_TRACK_OFFSET + HIMD_NUM_TRACKS * SIZEOF_TRACK_CHUNK:
        chunk = tifdata[trkoff:trkoff+SIZEOF_TRACK_CHUNK]

        (recording_date, recording_time, ekbnum, title_idx, artist_idx, album_idx,
         track_in_album, unknownbyte15, key, mac, codec_id, codec_info_head,
         firstfrag, track_number, duration_seconds, lt, dest,
         codec_info_tail, unknownbytes46, content_id,
         license_start_date, license_start_time,
         license_end_date, license_end_time,
         xcc, ct, cc, cn) = struct.unpack('>HHIHHHBB8s8sB3sHHHBB2s2s20sHHHHBBBB', chunk)

        date, time = parse_dostime(recording_date, recording_time)
        license_start_date, license_start_time = parse_dostime(license_start_date, license_start_time)
        license_end_date, license_end_time = parse_dostime(license_end_date, license_end_time)

        print(f'Track chunk {i}: rectime: {date} {time} {ekbnum=:08x} '
              f'{title_idx=} {artist_idx=} {album_idx=} {track_in_album=} {unknownbyte15=} '
              f'key={binascii.hexlify(key).decode()} mac={binascii.hexlify(mac).decode()} '
              f'\n\tcodec={CODECS[codec_id]:10s} (codec=[{binascii.hexlify(codec_info_head).decode()}, '
              f'{binascii.hexlify(codec_info_tail).decode()}] '
              f'{firstfrag=} {track_number=} {duration_seconds=} {lt=} {dest=} {unknownbytes46=} '
              f'\n\tcontent_id={binascii.hexlify(content_id).decode()} '
              f'license=(start ({license_start_date} {license_start_time}-'
              f'end {license_end_date} {license_end_time}) {xcc=} {ct=} {cc=} {cn=}')

        #print(binascii.hexlify(chunk))

        i += 1
        trkoff += SIZEOF_TRACK_CHUNK

if args.fragments:
    frgoff = HIMD_FRAGMENT_OFFSET
    i = 0
    while frgoff < HIMD_FRAGMENT_OFFSET + HIMD_NUM_FRAGMENTS * SIZEOF_FRAGMENT_CHUNK:
        chunk = tifdata[frgoff:frgoff+SIZEOF_FRAGMENT_CHUNK]

        (key, first_block, last_block, first_frame, last_frame, next_frag_fragtype) = struct.unpack('>8sHHBBH', chunk)

        fragtype = (next_frag_fragtype >> 12) & 0b1111
        next_frag = next_frag_fragtype & 0x0FFF

        print(f'Fragment chunk {i}: {first_block=} {last_block=} {first_frame=} {last_frame=} {fragtype=} {next_frag=}')

        i += 1
        frgoff += SIZEOF_FRAGMENT_CHUNK

if args.strings:
    stroff = HIMD_STRING_OFFSET
    i = 0
    while stroff < HIMD_STRING_OFFSET + HIMD_NUM_STRINGS * SIZEOF_STRING_CHUNK:
        chunk = tifdata[stroff:stroff+SIZEOF_STRING_CHUNK]
        (stringtype_nextstring,) = struct.unpack('>H', chunk[14:])
        stringtype = (stringtype_nextstring >> 12) & 0b1111
        nextstring = stringtype_nextstring & 0b111111111111

        if stringtype not in (STRING_TYPE_UNUSED, STRING_TYPE_CONTINUATION):
            encoding = ENCODING_TYPES[chunk[0]]
            txt = chunk[1:14]
        else:
            txt = chunk[0:14]
            encoding = 'n/a'

        print(f'String chunk {i} (0x{i:04x}): {binascii.hexlify(chunk)}, {txt}, '
              f'type: {STRING_TYPES.get(stringtype)}, '
              f'encoding: {encoding}, '
              f'next: {nextstring}')

        i += 1
        stroff += SIZEOF_STRING_CHUNK
