#!/usr/bin/python
import usb1
import libnetmd

def main(bus=None, device_address=None):
    context = usb1.LibUSBContext()
    for md in libnetmd.iterdevices(context, bus=bus,
                                   device_address=device_address):
        MDctl(md)

def MDctl(md):
    md_iface = libnetmd.NetMDInterface(md)
    import pdb;
    pdb.set_trace()

if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option('-b', '--bus')
    parser.add_option('-d', '--device')
    (options, args) = parser.parse_args()
    assert len(args) == 0

    main(bus=options.bus, device_address=options.device)

