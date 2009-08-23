#!/usr/bin/python
import os
import usb1
import libnetmd
from time import sleep

def main(bus=None, device_address=None, ext='ogg', track_range=None):
    context = usb1.LibUSBContext()
    for md in libnetmd.iterdevices(context, bus=bus,
                                   device_address=device_address):
        md_iface = libnetmd.NetMDInterface(md)
        try:
            MDDump(md_iface, ext, track_range)
        finally:
            md_iface.stop()

def getTrackList(md_iface, track_range):
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
        hour, minute, second, sample = md_iface.getTrackLength(track)
        codec, channel_count = md_iface.getTrackEncoding(track)
        channel_count = libnetmd.CHANNEL_COUNT_DICT[channel_count]
        ascii_title = md_iface.getTrackTitle(track)
        wchar_title = md_iface.getTrackTitle(track, True).decode('shift_jis')
        title = wchar_title or ascii_title
        append((track,
                (hour, minute, second, sample),
                str(channel_count),
                title))
    return result

def MDDump(md_iface, ext, track_range):
    ascii_title = md_iface.getDiscTitle()
    wchar_title = md_iface.getDiscTitle(True).decode('shift_jis')
    disc_title = wchar_title or ascii_title
    print 'Storing in', disc_title
    if not os.path.exists(disc_title):
        os.mkdir(disc_title)
    for track, (hour, minute, second, sample), channels, title in \
        getTrackList(md_iface, track_range):

        duration = '%02i:%02i:%02i.%03i' % (hour, minute, second, sample/.512)
        filename = '%02i - %s.%s' % (track + 1, title, ext)
        print 'Recording', filename, '(', duration, ')'
        md_iface.gotoTrack(track)
        # Attemp to reduce the MD play delay by...
        print 'Waiting for MD...'
        # Waiting for the seek to complete...
        while md_iface.getPosition() != [track, 0, 0, 0, 1]:
            sleep(1)
        # And waiting for the play to actualy begin.
        md_iface.play()
        while md_iface.getPosition() < [track, 0, 0, 1, 1]:
            sleep(0.5)
        # Pause and go back to track beginning.
        md_iface.pause()
        md_iface.gotoTrack(track)
        pid = os.fork()
        if pid == 0:
            os.execlp('sox', 'sox',
                      '-d',
                      '-S',
                      '-c', channels,
                      '%s/%s' % (disc_title, filename),
                      'silence', '1', '0.1', '0.1%',
                      'trim', '0', duration,
            )
        else:
            md_iface.play()
            sleep(((hour * 60 + minute) * 60) + second)
            while md_iface.getPosition()[0] == track:
                sleep(1)
            md_iface.pause()
            print 'Done, waiting for sox to return...'
            os.waitpid(pid, 0)
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
    if len(args) == 1:
        ext = args[0]
    else:
        ext = 'ogg'
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
    main(bus=options.bus, device_address=options.device, ext=ext,
         track_range=track_range)

