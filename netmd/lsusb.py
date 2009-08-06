#!/usr/bin/python
import usb1
import sys

def main():
    verbose = False
    filter_set = set()

    for arg in sys.argv:
        if arg == '-v':
            verbose = True
        elif ':' in arg:
            vendor, product = arg.split(':')
            filter_set.add((int(vendor, 16), int(product, 16)))

    if len(filter_set):
        def test(device):
            return (device.getVendorID(), device.getProductID()) in filter_set
    else:
        def test(device):
            return True
    context = usb1.LibUSBContext()
    for device in context.getDeviceList():
        if test(device):
            print device
            if verbose:
                print device.reprConfigurations()

if __name__ == '__main__':
    main()

