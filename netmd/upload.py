#!/usr/bin/python
import os
import usb1
import libnetmd
from time import sleep
from struct import pack

RIFF_FORMAT_TAG_ATRAC3 = 0x270

UPLOAD_FORMAT_LP4 = 0x80
UPLOAD_FORMAT_LP2 = 0x82
UPLOAD_FORMAT_SP = 0x86

# This creates an ffmpeg compatible WAV header.
class wavUploadEvents(libnetmd.defaultUploadEvents):
    def __init__(self, stream, channels):
        self.stream = stream
        self.channels = channels
    
    def trackinfo(self, frames, bytes, format):
        print 'Format byte', format
        if format == UPLOAD_FORMAT_LP4:
            bytesperframe = 96
            jointstereo = 1
        else:
            bytesperframe = 192
            jointstereo = 0
        bytespersecond = bytesperframe * 512 / 44100
        # RIFF header
        self.stream.write(pack("<4sI4s", 'RIFF', bytes+60, 'WAVE'))
        # standard fmt chunk
        self.stream.write(pack("<4sIHHIIHH", 'fmt ',32, RIFF_FORMAT_TAG_ATRAC3,
                          self.channels, 44100, bytespersecond, 
                          self.channels * bytesperframe, 0))
        # ATRAC specific extra data
        self.stream.write(pack("<HHIHHHH", 14, 1, bytesperframe,
                                           jointstereo, jointstereo, 1, 0))
        # data chunk header
        self.stream.write(pack("<4sI", 'data', bytes))
        libnetmd.defaultUploadEvents.trackinfo(self, frames, bytes, format)

def main(bus=None, device_address=None, track_range=None):
    context = usb1.LibUSBContext()
    for md in libnetmd.iterdevices(context, bus=bus,
                                   device_address=device_address):
        md_iface = libnetmd.NetMDInterface(md)
        MDDump(md_iface, track_range)

def getTrackList(md_iface, track_range):
    channel_count_dict = {
        libnetmd.CHANNELS_MONO: 1,
        libnetmd.CHANNELS_STEREO: 2,
    }
    result = []
    append = result.append
    track_count = md_iface.getTrackCount()
    if isinstance(track_range, tuple):
        min_track, max_track = track_range
        if max_track is None:
            max_track = track_count - 1
        assert max_track < track_count
        assert min_track < track_count
        track_list = xrange(min_track, max_track + 1)
    elif isinstance(track_range, int):
        assert track_range < track_count
        track_list = [track_range]
    else:
        track_list = xrange(track_count)
    for track in track_list:
        flags = md_iface.getTrackFlags(track)
        codec, channel_count = md_iface.getTrackEncoding(track)
        if flags != libnetmd.TRACK_FLAG_PROTECTED:
            channel_count = libnetmd.CHANNEL_COUNT_DICT[channel_count]
            ascii_title = md_iface.getTrackTitle(track)
            wchar_title = md_iface.getTrackTitle(track, True).decode('shift_jis')
            title = wchar_title or ascii_title
            append((track, codec,
                    channel_count,
                    title))
    return result

def formatAeaHeader(name = '', channels = 2, soundgroups = 1, groupstart = 0, encrypted = 0, flags=[0,0,0,0,0,0,0,0]):
    return pack("2048s", # pad to header size
                pack("<I256siBx8IIII"
                                      ,2048,   # header size
                                      name,
                                      soundgroups,
                                      channels,
                                      flags[0],flags[1],flags[2],flags[3],
                                      flags[4],flags[5],flags[6],flags[7],
                                      0,      # Should be time of recordin in
                                              # 32 bit DOS format.
                                      encrypted,
                                      groupstart
                                      ))

def MDDump(md_iface, track_range):
    ascii_title = md_iface.getDiscTitle()
    wchar_title = md_iface.getDiscTitle(True).decode('shift_jis')
    disc_title = wchar_title or ascii_title
    if disc_title == '':
        directory = '.'
    else:
        directory = disc_title;
    print 'Storing in', directory
    if not os.path.exists(directory):
        os.mkdir(directory)
    for track, codec, channels, title in \
        getTrackList(md_iface, track_range):

        if codec == libnetmd.ENCODING_SP:
            extension = 'aea'
        else:
            extension = 'wav'
        filename = '%02i - %s.%s' % (track + 1, title, extension)
        print 'Uploading', filename
        aeafile = open(filename,"w")
        if codec == libnetmd.ENCODING_SP:
            aeafile.write(formatAeaHeader(channels=channels))
            md_iface.saveTrackToStream(track, aeafile)
        else:
            md_iface.saveTrackToStream(track, aeafile,events=wavUploadEvents(aeafile, channels))

    # TODO: generate playlists based on groups defined on the MD
    print 'Finished.'

if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('-b', '--bus')
    parser.add_option('-d', '--device')
    parser.add_option('-t', '--track-range')
    (options, args) = parser.parse_args()
    assert len(args) < 2
    track_range = options.track_range
    if track_range is not None:
        if '-' in track_range:
            begin, end = track_range.split('-', 1)
            if begin == '':
                begin = 0
            else:
                begin = int(begin) - 1
            if end == '':
                end = None
            else:
                end = int(end) - 1
                assert begin < end
            track_range = (begin, end)
        else:
            track_range = int(track_range) - 1
    main(bus=options.bus, device_address=options.device,
         track_range=track_range)

