#!/usr/bin/python
import usb1
import libnetmd

def main(bus=None, device_address=None, show_uuids=False):
    context = usb1.LibUSBContext()
    for md in libnetmd.iterdevices(context, bus=bus,
                                   device_address=device_address):
        listMD(md, show_uuids)

def listMD(md, show_uuids):
    md_iface = libnetmd.NetMDInterface(md)

    codec_name_dict = {
        libnetmd.ENCODING_SP: 'sp',
        libnetmd.ENCODING_LP2: 'lp2',
        libnetmd.ENCODING_LP4: 'lp4', 
    }
    channel_count_dict = {
        libnetmd.CHANNELS_MONO: 'mono',
        libnetmd.CHANNELS_STEREO: 'stereo',
    }
    flag_dict = {
        libnetmd.TRACK_FLAG_PROTECTED: 'protected',
        0: 'unprotected',
    }
    def reprDiscFlags(flags):
        result = []
        if flags & libnetmd.DISC_FLAG_WRITABLE:
            result.append('writable media')
        if flags & libnetmd.DISC_FLAG_WRITE_PROTECTED:
            result.append('write-protected')
        return result

    def timeToFrames(time_tuple):
        assert len(time_tuple) == 4
        return (((time_tuple[0] * 60) + time_tuple[1]) * 60 + time_tuple[2]) \
            * 512 + time_tuple[3]

    flags = reprDiscFlags(md_iface.getDiscFlags())
    print 'Disk (%s) %s %s' % (
        ', '.join(flags), md_iface.getDiscTitle(),
        md_iface.getDiscTitle(True).decode('shift_jis_2004'))
    disc_used, disc_total, disc_left = md_iface.getDiscCapacity()
    disc_total = timeToFrames(disc_total)
    disc_left = timeToFrames(disc_left)
    print 'Time used: %02i:%02i:%02i+%03i (%.02f%%)' % (
        disc_used[0], disc_used[1], disc_used[2], disc_used[3],
        (disc_total - disc_left) / float(disc_total) * 100)
    track_count = md_iface.getTrackCount()
    print '%i tracks' % (track_count, )
    for group, (group_name, track_list) in enumerate(
        md_iface.getTrackGroupList()):
        if group_name is None:
            prefix = ''
        else:
            prefix = '  '
            print 'Group %r' % (group_name or group + 1, )
        for track, real_track in enumerate(track_list):
            hour, minute, second, sample = md_iface.getTrackLength(real_track)
            codec, channel_count = md_iface.getTrackEncoding(real_track)
            flags = md_iface.getTrackFlags(real_track)
            print '%s%03i: %02i:%02i:%02i+%03i %s %s %s %s %s' % (prefix,
                track, hour, minute, second, sample, codec_name_dict[codec],
                channel_count_dict[channel_count], flag_dict[flags],
                md_iface.getTrackTitle(real_track).decode('shift_jis_2004'),
                md_iface.getTrackTitle(real_track, True).decode('shift_jis_2004'))
            if show_uuids:
                uuid = md_iface.getTrackUUID(real_track)
                print '%s UUID:' % prefix, ''.join(["%02x"%ord(i) for i in uuid])

if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('-b', '--bus')
    parser.add_option('-d', '--device')
    parser.add_option('-u', '--uuids', action="store_true")
    (options, args) = parser.parse_args()
    assert len(args) == 0

    main(bus=options.bus, device_address=options.device, 
         show_uuids=options.uuids)

