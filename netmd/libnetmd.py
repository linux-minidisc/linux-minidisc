import libusb1
from cStringIO import StringIO

def dump(data):
    if isinstance(data, basestring):
        result = ' '.join(['%02x' % (ord(x), ) for x in data])
    else:
        result = repr(data)
    return result

KNOWN_USB_ID_SET = frozenset([
    (0x054c, 0x0075), # Sony MZ-N1 
    (0x054c, 0x0080), # Sony LAM-1 
    (0x054c, 0x0081), # Sony MDS-JB980 
    (0x054c, 0x0084), # Sony MZ-N505 
    (0x054c, 0x0085), # Sony MZ-S1 
    (0x054c, 0x0086), # Sony MZ-N707 
    (0x054c, 0x00c6), # Sony MZ-N10 
    (0x054c, 0x00c8), # Sony MZ-N710/N810 
    (0x054c, 0x00c9), # Sony MZ-N510/N610 
    (0x054c, 0x00ca), # Sony MZ-NE410/NF520D 
    (0x054c, 0x00c7), # Sony MZ-N910
    (0x054c, 0x0286), # Sony MZ-RH1
])

def iterdevices(usb_context, bus=None, device_address=None):
    for device in usb_context.getDeviceList():
        if bus is not None and bus != device.getBusNumber():
            continue
        if device_address is not None and \
           device != device_address.getDeviceAddress():
            continue
        if (device.getVendorID(), device.getProductID()) in KNOWN_USB_ID_SET:
            yield NetMD(device.open())

# XXX: Endpoints numbers are hardcoded
BULK_WRITE_ENDPOINT = 0x02
BULK_READ_ENDPOINT = 0x81

# NetMD Protocol return status (first byte of request)
STATUS_CONTROL = 0x00
STATUS_STATUS = 0x01
STATUS_SPECIFIC_INQUIRY = 0x02
STATUS_NOTIFY = 0x03
STATUS_GENERAL_INQUIRY = 0x04
# ... (first byte of response)
STATUS_NOT_IMPLEMENTED = 0x08
STATUS_ACCEPTED = 0x09
STATUS_REJECTED = 0x0a
STATUS_IN_TRANSITION = 0x0b
STATUS_IMPLEMENTED = 0x0c
STATUS_CHANGED = 0x0d
STATUS_INTERIM = 0x0f

class NetMDException(Exception):
    pass

class NetMDNotImplemented(NetMDException):
    pass

class NetMDRejected(NetMDException):
    pass

class NetMD(object):
    def __init__(self, usb_handle, interface=0):
        self.usb_handle = usb_handle
        self.interface = interface
        usb_handle.setConfiguration(1)
        usb_handle.claimInterface(interface)

    def __del__(self):
        try:
            self.usb_handle.releaseInterface(self.interface)
        except: # Should specify an usb exception
            pass

    def _getReplyLength(self):
        reply = self.usb_handle.controlRead(libusb1.LIBUSB_TYPE_VENDOR | \
                                            libusb1.LIBUSB_RECIPIENT_INTERFACE,
                                            0x01, 0, 0, 4)
        return ord(reply[2])

    def sendCommand(self, command):
        #print '%04i> %s' % (len(command), dump(command))
        self.usb_handle.controlWrite(libusb1.LIBUSB_TYPE_VENDOR | \
                                     libusb1.LIBUSB_RECIPIENT_INTERFACE,
                                     0x80, 0, 0, command)

    def readReply(self):
        reply_length = self._getReplyLength()
        reply = self.usb_handle.controlRead(libusb1.LIBUSB_TYPE_VENDOR | \
                                            libusb1.LIBUSB_RECIPIENT_INTERFACE,
                                            0x81, 0, 0, reply_length)
        #print '%04i< %s' % (len(reply), dump(reply))
        return reply

    def readBulk(self, length, chunk_size=0x10000):
        result = StringIO()
        self.readBulkToFile(length, result, chunk_size=chunk_size)
        return result.getvalue()

    def readBulkToFile(self, length, outfile, chunk_size=0x10000):
        done = 0
        while done < length:
            received = self.usb_handle.bulkRead(BULK_READ_ENDPOINT,
                min((length - done), chunk_size))
            done += len(received)
            outfile.write(received)
            print 'Done: %x/%x (%.02f%%)' % (done, length,
                                             done/float(length) * 100)

    def writeBulk(self, data):
        self.usb_handle.bulkWrite(BULK_WRITE_ENDPOINT, data)

ACTION_PLAY = 0x75
ACTION_PAUSE = 0x7d
ACTION_FASTFORWARD = 0x39
ACTION_REWIND = 0x49

TRACK_PREVIOUS = 0x0002
TRACK_NEXT = 0x8001
TRACK_RESTART = 0x0001

ENCODING_SP = 0x90
ENCODING_LP2 = 0x92
ENCODING_LP4 = 0x93

CHANNELS_MONO = 0x01
CHANNELS_STEREO = 0x00

OPERATING_STATUS_USB_RECORDING = 0x56ff
OPERATING_STATUS_RECORDING = 0xc275
OPERATING_STATUS_RECORDING_PAUSED = 0xc27d
OPERATING_STATUS_FAST_FORWARDING = 0xc33f
OPERATING_STATUS_REWINDING = 0xc34f
OPERATING_STATUS_PLAYING = 0xc375
OPERATING_STATUS_PAUSED = 0xc37d
OPERATING_STATUS_STOPPED = 0xc5ff

TRACK_FLAG_PROTECTED = 0x03

DISC_FLAG_WRITABLE = 0x10
DISC_FLAG_WRITE_PROTECTED = 0x40

_FORMAT_TYPE_LEN_DICT = {
    'b': 1, # byte
    'w': 2, # word
    'd': 4, # doubleword
    'q': 8, # quadword
}

def BCD2int(bcd, length=1):
    value = 0
    for nibble in xrange(length * 2):
        nibble_value = bcd & 0xf
        bcd >>= 4
        value += nibble_value * (10 ** nibble)
    return value

def int2BCD(value, length=1):
    if value > 10 ** (length * 2 - 1):
        raise ValueError, 'Value %r cannot fit in %i bytes in BCD' % \
             (value, length)
    bcd = 0
    for nibble in xrange(length * 2):
        nibble_value = value % 10
        value /= 10
        bcd |= nibble_value << (4 * nibble)
    return bcd

class NetMDInterface(object):
    def __init__(self, net_md):
        self.net_md = net_md

    def send_query(self, query, test=False):
        # XXX: to be removed (replaced by 2 separate calls)
        self.sendCommand(query, test=test)
        return self.readReply()

    def sendCommand(self, query, test=False):
        if test:
            query = [STATUS_SPECIFIC_INQUIRY, ] + query
        else:
            query = [STATUS_CONTROL, ] + query
        binquery = ''.join(chr(x) for x in query)
        self.net_md.sendCommand(binquery)

    def readReply(self):
        result = self.net_md.readReply()
        status = ord(result[0])
        if status == STATUS_NOT_IMPLEMENTED:
            raise NetMDNotImplemented, 'Not implemented'
        elif status == STATUS_REJECTED:
            raise NetMDRejected, 'Rejected'
        elif status not in (STATUS_ACCEPTED, STATUS_IMPLEMENTED,
                            STATUS_INTERIM):
            raise NotImplementedError, 'Unknown returned status: %02X' % \
                                       (status, )
        return result[1:]

    def formatQuery(self, format, *args):
        result = []
        append = result.append
        extend = result.extend
        half = None
        def hexAppend(value):
            append(int(value, 16))
        escaped = False
        arg_stack = list(args)
        for char in format:
            if escaped:
                escaped = False
                value = arg_stack.pop(0)
                if char in _FORMAT_TYPE_LEN_DICT:
                    for byte in xrange(_FORMAT_TYPE_LEN_DICT[char] - 1, -1, -1):
                        append((value >> (byte * 8)) & 0xff)
                # String ('s' is 0-terminated, 'x' is not)
                elif char in ('s', 'x'):
                    length = len(value)
                    if char == 's':
                        length += 1
                    append((length >> 8) & 0xff)
                    append(length & 0xff)
                    extend(ord(x) for x in value)
                    if char == 's':
                        append(0)
                else:
                    raise ValueError, 'Unrecognised format char: %r' % (char, )
                continue
            if char == '%':
                assert half is None
                escaped = True
                continue
            if char == ' ':
                continue
            if half is None:
                half = char
            else:
                hexAppend(half + char)
                half = None
        assert len(arg_stack) == 0
        return result

    def scanQuery(self, query, format):
        result = []
        append = result.append
        half = None
        escaped = False
        input_stack = list(query)
        def pop():
            return ord(input_stack.pop(0))
        for char in format:
            if escaped:
                escaped = False
                if char == '?':
                    pop()
                    continue
                if char in _FORMAT_TYPE_LEN_DICT:
                    value = 0
                    for byte in xrange(_FORMAT_TYPE_LEN_DICT[char] - 1, -1, -1):
                        value |= (pop() << (byte * 8))
                    append(value)
                # String ('s' is 0-terminated, 'x' is not)
                elif char in ('s', 'x'):
                    length = pop() << 8 | pop()
                    value = ''.join(input_stack[:length])
                    input_stack = input_stack[length:]
                    if char == 's':
                        append(value[:-1])
                    else:
                        append(value)
                # Fetch the remainder of the query in one value
                elif char == '*':
                    value = ''.join(input_stack)
                    input_stack = []
                    append(value)
                else:
                    raise ValueError, 'Unrecognised format char: %r' % (char, )
                continue
            if char == '%':
                assert half is None
                escaped = True
                continue
            if char == ' ':
                continue
            if half is None:
                half = char
            else:
                input_value = pop()
                format_value = int(half + char, 16)
                if format_value != input_value:
                    raise ValueError, 'Format and input mismatch at %i: ' \
                                      'expected %02x, got %02x' % \
                                      (len(query) - len(input_stack) - 1,
                                       format_value, input_value)
                half = None
        assert len(input_stack) == 0
        return result

    def acquire(self):
        query = self.formatQuery('ff 010c ffff ffff ffff ffff ffff ffff')
        reply = self.send_query(query)
        self.scanQuery(reply, 'ff 010c ffff ffff ffff ffff ffff ffff')

    def release(self):
        query = self.formatQuery('ff 0100 ffff ffff ffff ffff ffff ffff')
        reply = self.send_query(query)
        self.scanQuery(reply, 'ff 0100 ffff ffff ffff ffff ffff ffff')

    def getStatus(self):
        query = self.formatQuery('1809 8001 0230 8800 0030 8804 00 ff00 ' \
                                 '00000000')
        reply = self.send_query(query)
        return self.scanQuery(reply, '1809 8001 0230 8800 0030 8804 00 ' \
                              '1000 000900000 %x')[0]

    def isDiskPresent(self):
        status = self.getStatus()
        return status[4] == 0x40

    def getOperatingStatus(self):
        query = self.formatQuery('1809 8001 0330 8802 0030 8805 0030 8806 ' \
                                 '00 ff00 00000000')
        reply = self.send_query(query)
        return self.scanQuery(reply, '1809 8001 0330 8802 0030 8805 0030 ' \
                              '8806 00 1000 00%?0000 0006 8806 0002 %w')[0]

    def _getPlaybackStatus(self, p1, p2):
        query = self.formatQuery('1809 8001 0330 %w 0030 8805 0030 %w 00 ' \
                                 'ff00 00000000',
                                 p1, p2)
        reply = self.send_query(query)
        return self.scanQuery(reply, '1809 8001 0330 %?%? %?%? %?%? %?%? ' \
                              '%?%? %? 1000 00%?0000 %x')[0]

    def getPlaybackStatus1(self):
        return self._getPlaybackStatus(0x8801, 0x8807)

    def getPlaybackStatus2(self):
        # XXX: duplicate of getOperatingStatus
        return self._getPlaybackStatus(0x8802, 0x8806)

    def getPosition(self):
        query = self.formatQuery('1809 8001 0430 8802 0030 8805 0030 0003 ' \
                                 '0030 0002 00 ff00 00000000')
        try:
            reply = self.send_query(query)
        except NetMDRejected: # No disc
            result = None
        else:
            result = self.scanQuery(reply, '1809 8001 0430 %?%? %?%? %?%? ' \
                                    '%?%? %?%? %?%? %?%? %? %?00 00%?0000 ' \
                                    '000b 0002 0007 00 %w %b %b %b %b')
            result[1] = BCD2int(result[1])
            result[2] = BCD2int(result[2])
            result[3] = BCD2int(result[3])
            result[4] = BCD2int(result[4])
        return result

    def _play(self, action):
        query = self.formatQuery('18c3 ff %b 000000', action)
        reply = self.send_query(query)
        self.scanQuery(reply, '18c3 00 %b 000000')

    def play(self):
        self._play(ACTION_PLAY)

    def fast_forward(self):
        self._play(ACTION_FASTFORWARD)

    def rewind(self):
        self._play(ACTION_REWIND)

    def pause(self):
        self._play(ACTION_PAUSE)

    def stop(self):
        query = self.formatQuery('18c5 ff 00000000')
        reply = self.send_query(query)
        self.scanQuery(reply, '18c5 00 00000000')

    def gotoTrack(self, track):
        query = self.formatQuery('1850 ff010000 0000 %w', track)
        reply = self.send_query(query)
        return self.scanQuery(reply, '1850 00010000 0000 %w')[0]

    def gotoTime(self, track, hour=0, minute=0, second=0, frame=0):
        query = self.formatQuery('1850 ff000000 0000 %w %b%b%b%b', track,
                                 int2BCD(hour), int2BCD(minute),
                                 int2BCD(second), int2BCD(frame))
        reply = self.send_query(query)
        return self.scanQuery(reply, '1850 00000000 %?%? %w %b%b%b%b')

    def _trackChange(self, direction):
        query = self.formatQuery('1850 ff10 00000000 %w', direction)
        reply = self.send_query(query)
        return self.scanQuery(reply, '1850 0010 00000000 %?%?')

    def nextTrack(self):
        self._trackChange(TRACK_NEXT)

    def previousTrack(self):
        self._trackChange(TRACK_PREVIOUS)

    def restartTrack(self):
        self._trackChange(TRACK_RESTART)

    def eraseDisc(self):
        query = self.formatQuery('1840 ff 0000')
        reply = self.send_query(query)
        self.scanQuery(reply, '1840 00 0000')

#    def syncTOC(self):
#        query = self.formatQuery('1808 10180200 00')
#        reply = self.send_query(query)
#        return self.scanQuery(reply, '1808 10180200 00')

#    def cacheTOC(self):
#        query = self.formatQuery('1808 10180203 00')
#        reply = self.send_query(query)
#        return self.scanQuery(reply, '1808 10180203 00')

    def getDiscFlags(self):
        query = self.formatQuery('1806 01101000 ff00 0001000b')
        reply = self.send_query(query)
        return self.scanQuery(reply, '1806 01101000 1000 0001000b %b')[0]

    def getTrackCount(self):
        query = self.formatQuery('1806 02101001 3000 1000 ff00 00000000')
        reply = self.send_query(query)
        data = self.scanQuery(reply, '1806 02101001 %?%? %?%? 1000 00%?0000 ' \
                              '%x')[0]
        assert len(data) == 6, len(data)
        assert data[:5] == '\x00\x10\x00\x02\x00', data[:5]
        return ord(data[5])

    def _getDiscTitle(self, wchar=False):
        if wchar:
            wchar_value = 1
        else:
            wchar_value = 0
        done = 0
        remaining = 0
        total = 1
        result = []
        while done < total:
            query = self.formatQuery('1806 02201801 00%b 3000 0a00 ff00 %w%w',
                                     wchar_value, remaining, done)
            reply = self.send_query(query)
            if remaining == 0:
                chunk_size, total, chunk = self.scanQuery(reply,
                    '1806 02201801 00%? 3000 0a00 1000 %w0000 %?%?000a %w %*')
                chunk_size -= 6
            else:
                chunk_size, chunk = self.scanQuery(reply,
                    '1806 02201801 00%? 3000 0a00 1000 %w%?%? %*')
            assert chunk_size == len(chunk)
            result.append(chunk)
            done += chunk_size
            remaining = total - done
        #if not wchar and len(result):
        #    assert result[-1] == '\x00'
        #    result = result[:-1]
        return ''.join(result)

    def getDiscTitle(self, wchar=False):
        raw_title = self._getDiscTitle(wchar=wchar)
        title = raw_title.split('//')[0]
        if ';' in title and title[0] == '0':
            title = title.split(';')[-1]
        return title

    def getTrackGroupDict(self):
        raw_title = self._getDiscTitle()
        group_list = raw_title.split('//')
        result = {}
        for group_index, group in enumerate(group_list):
            if group == '': # (only ?) last group might be delimited but empty.
                continue
            if group[0] == '0' or ';' not in group: # Disk title
                continue
            track_range, group_name = group.split(';', 1)
            if '-' in track_range:
                track_min, track_max = track_range.split('-')
                assert track_min < track_max, '%r, %r' % (track_min, track_max)
            else:
                track_min = track_max = track_range
            for track in xrange(int(track_min) - 1, int(track_max)):
                if track in result:
                    raise ValueError, 'Track %i is in 2 groups: %r[%i] & ' \
                         '%r[%i]' % (track, result[track][0], result[track][1],
                         group_name, group_index)
                result[track] = group_name, group_index
        return result

    def getTrackTitle(self, track, wchar=False):
        if wchar:
            wchar_value = 3
        else:
            wchar_value = 2
        query = self.formatQuery('1806 022018%b %w 3000 0a00 ff00 00000000',
                                 wchar_value, track)
        reply = self.send_query(query)
        result = self.scanQuery(reply, '1806 022018%? %?%? %?%? %?%? 1000 ' \
                                '00%?0000 00%?000a %x')[0]
        #if not wchar and len(result):
        #    assert result[-1] == '\x00'
        #    result = result[:-1]
        return result

    def setDiscTitle(self, title, wchar=False):
        if wchar:
            wchar = 1
        else:
            wchar = 0
        old_len = len(self.getDiscTitle())
        query = self.formatQuery('1807 02201801 00%b 3000 0a00 5000 %w 0000 ' \
                                 '%w %s', wchar, len(title), old_len, title)
        reply = self.send_query(query)
        self.scanQuery(reply, '1807 02201801 00%? 3000 0a00 5000 %?%? 0000 ' \
                              '%?%?')

    def setTrackTitle(self, track, title, wchar=False):
        if wchar:
            wchar = 3
        else:
            wchar = 2
        old_len = len(self.getTrackTitle(track))
        query = self.formatQuery('1807 022018%b %w 3000 0a00 5000 %w 0000 ' \
                                 '%w %s', wchar, track, len(title), old_len,
                                 title)
        reply = self.send_query(query)
        self.scanQuery(reply, '1807 022018%? %?%? 3000 0a00 5000 %?%? 0000 ' \
                              '%?%?')

    def eraseTrack(self, track):
        query = self.formatQuery('1840 ff01 00 201001 %w', track)
        reply = self.send_query(query)
        self.scanQuery(reply, '1840 1001 00 201001 %?%?')

    def moveTrack(self, source, dest):
        query = self.formatQuery('1843 ff00 00 201001 00 %w 201001 %w', source,
                                 dest)
        reply = self.send_query(query)
        self.scanQuery(reply, '1843 0000 00 201001 00 %?%? 201001 %?%?')

    def _getTrackInfo(self, track, p1, p2):
        query = self.formatQuery('1806 02201001 %w %w %w ff00 00000000', track,
                                 p1, p2)
        reply = self.send_query(query)
        return self.scanQuery(reply, '1806 02201001 %?%? %?%? %?%? 1000 ' \
                              '00%?0000 %x')[0]

    def getTrackLength(self, track):
        raw_value = self._getTrackInfo(track, 0x3000, 0x0100)
        result = self.scanQuery(raw_value, '0001 0006 0000 %b %b %b %b')
        result[0] = BCD2int(result[0])
        result[1] = BCD2int(result[1])
        result[2] = BCD2int(result[2])
        result[3] = BCD2int(result[3])
        return result

    def getTrackEncoding(self, track):
        return self.scanQuery(self._getTrackInfo(track, 0x3080, 0x0700),
                              '8007 0004 0110 %b %b')

    def getTrackFlags(self, track):
        query = self.formatQuery('1806 01201001 %w ff00 00010008', track)
        reply = self.send_query(query)
        return self.scanQuery(reply, '1806 01201001 %?%? 10 00 00010008 %b') \
               [0]

    def getDiscCapacity(self):
        query = self.formatQuery('1806 02101000 3080 0300 ff00 00000000')
        reply = self.send_query(query)
        raw_result = self.scanQuery(reply, '1806 02101000 3080 0300 1000 ' \
                                    '001d0000 001b 8003 0017 8000 0005 %w ' \
                                    '%b %b %b 0005 %w %b %b %b 0005 %w %b ' \
                                    '%b %b')
        result = []
        for offset in xrange(3):
            offset *= 4
            result.append([
                BCD2int(raw_result[offset + 0], 2),
                BCD2int(raw_result[offset + 1]),
                BCD2int(raw_result[offset + 2]),
                BCD2int(raw_result[offset + 3])])
        return result

    def getRecordingParameters(self):
        query = self.formatQuery('1809 8001 0330 8801 0030 8805 0030 8807 ' \
                                 '00 ff00 00000000')
        reply = self.send_query(query)
        return self.scanQuery(reply, '1809 8001 0330 8801 0030 8805 0030 ' \
                              '8807 00 1000 000e0000 000c 8805 0008 80e0 ' \
                              '0110 %b %b 4000')

    def saveTrackToFile(self, track, outfile_name):
        track += 1
        query = self.formatQuery('1800 080046 f003010330 ff00 1001 %w', track)
        reply = self.send_query(query)
        length = self.scanQuery(reply, '1800 080046 f003010330 0000 1001 ' \
                                '%?%? 86 %d')[0]
        self.net_md.readBulkToFile(length, open(outfile_name, 'w'))
        reply = self.readReply()
        self.scanQuery(reply, '1800 080046 f003010330 0000 1001 %?%? 0000')
        return buffer

