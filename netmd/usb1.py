# pyusb compatibility layer for libus-1.0
import libusb1
from ctypes import byref, create_string_buffer, c_int
from cStringIO import StringIO

__all__ = ['LibUSBContext']

# Default string length
STRING_LENGTH = 256

class USBDeviceHandle(object):
    def __init__(self, handle):
        self.handle = handle

    def __del__(self):
        libusb1.libusb_close(self.handle)

    def getConfiguration(self):
        configuration = c_int()
        result = libusb1.libusb_get_configuration(self.handle,
                                                  byref(configuration))
        if result:
            raise libusb1.USBError, result
        return configuration

    def setConfiguration(self, configuration):
        result = libusb1.libusb_set_configuration(self.handle, configuration)
        if result:
            raise libusb1.USBError, result

    def claimInterface(self, interface):
        result = libusb1.libusb_claim_interface(self.handle, interface)
        if result:
            raise libusb1.USBError, result

    def releaseInterface(self, interface):
        result = libusb1.libusb_release_interface(self.handle, interface)
        if result:
            raise libusb1.USBError, result

    def setInterfaceAltSetting(self, interface, alt_setting):
        result = libusb1.libusb_set_interface_alt_setting(self.handle,
                                                          interface,
                                                          alt_setting)
        if result:
            raise libusb1.USBError, result

    def clearHalt(self, endpoint):
        result = libusb1.libusb_clear_halt(self.handle, endpoint)
        if result:
            raise libusb1.USBError, result

    def resetDevice(self):
        result = libusb1.libusb_reset_device(self.handle)
        if result:
            raise libusb1.USBError, result

    def kernelDriverActive(self, interface):
        result = libusb1.libusb_kernel_driver_active(self.handle, interface)
        if result == 0:
            is_active = False
        elif result == 1:
            is_active = True
        else:
            raise libusb1.USBError, result
        return is_active

    def detachKernelDriver(self, interface):
        result = libusb1.libusb_detach_kernel_driver(self.handle, interface)
        if result:
            raise libusb1.USBError, result

    def attachKernelDriver(self, interface):
        result = libusb1.libusb_attach_kernel_driver(self.handle, interface)
        if result:
            raise libusb1.USBError, result

    # TODO: getStringDescriptor (in unicode)

    def getASCIIStringDescriptor(self, descriptor):
        descriptor_string = create_string_buffer(STRING_LENGTH)
        result = libusb1.libusb_get_string_descriptor_ascii(self.handle,
             descriptor, descriptor_string, STRING_LENGTH)
        if result < 0:
            raise libusb1.USBError, result
        return descriptor_string.value

    # Sync I/O

    def _controlTransfer(self, request_type, request, value, index, data,
                         length, timeout):
        result = libusb1.libusb_control_transfer(self.handle, request_type,
            request, value, index, data, length, timeout)
        if result < 0:
            raise libusb1.USBError, result
        return result

    def controlWrite(self, request_type, request, value, index, data,
                     timeout=0):
        request_type = (request_type & ~libusb1.USB_ENDPOINT_DIR_MASK) | \
                        libusb1.LIBUSB_ENDPOINT_OUT
        data = create_string_buffer(data)
        return self._controlTransfer(request_type, request, value, index, data,
                                     len(data)-1, timeout)

    def controlRead(self, request_type, request, value, index, length,
                    timeout=0):
        request_type = (request_type & ~libusb1.USB_ENDPOINT_DIR_MASK) | \
                        libusb1.LIBUSB_ENDPOINT_IN
        data = create_string_buffer(length)
        transferred = self._controlTransfer(request_type, request, value,
                                            index, data, length, timeout)
        return data.raw[:transferred]

    def _bulkTransfer(self, endpoint, data, length, timeout):
        transferred = c_int()
        result = libusb1.libusb_bulk_transfer(self.handle, endpoint,
            data, length, byref(transferred), timeout)
        if result:
            raise libusb1.USBError, result
        return transferred.value

    def bulkWrite(self, endpoint, data, timeout=0):
        endpoint = (endpoint & ~libusb1.USB_ENDPOINT_DIR_MASK) | \
                    libusb1.LIBUSB_ENDPOINT_OUT
        data = create_string_buffer(data)
        return self._bulkTransfer(endpoint, data, len(data) - 1, timeout)

    def bulkRead(self, endpoint, length, timeout=0):
        endpoint = (endpoint & ~libusb1.USB_ENDPOINT_DIR_MASK) | \
                    libusb1.LIBUSB_ENDPOINT_IN
        data = create_string_buffer(length)
        transferred = self._bulkTransfer(endpoint, data, length, timeout)
        return data.raw[:transferred]

    def _interruptTransfer(self, endpoint, data, length, timeout):
        transferred = c_int()
        result = libusb1.libusb_interrupt_transfer(self.handle, endpoint,
            data, length, byref(transferred), timeout)
        if result:
            raise libusb1.USBError, result
        return transferred.value

    def interruptWrite(self, endpoint, data, timeout=0):
        endpoint = (endpoint & ~libusb1.USB_ENDPOINT_DIR_MASK) | \
                    libusb1.LIBUSB_ENDPOINT_OUT
        data = create_string_buffer(data)
        return self._interruptTransfer(endpoint, data, len(data) - 1, timeout)

    def interruptRead(self, endpoint, length, timeout=0):
        endpoint = (endpoint & ~libusb1.USB_ENDPOINT_DIR_MASK) | \
                    libusb1.LIBUSB_ENDPOINT_IN
        data = create_string_buffer(length)
        transferred = self._interruptTransfer(endpoint, data, length, timeout)
        return data.raw[:transferred]

class USBDevice(object):

    configuration_descriptor_list = None

    def __init__(self, device_p):
        libusb1.libusb_ref_device(device_p)
        self.device_p = device_p
        # Fetch device descriptor
        device_descriptor = libusb1.libusb_device_descriptor()
        result = libusb1.libusb_get_device_descriptor(device_p,
            byref(device_descriptor))
        if result:
            raise libusb1.USBError, result
        self.device_descriptor = device_descriptor
        # Fetch all configuration descriptors
        self.configuration_descriptor_list = []
        append = self.configuration_descriptor_list.append
        for configuration_id in xrange(device_descriptor.bNumConfigurations):
            config = libusb1.libusb_config_descriptor_p()
            result = libusb1.libusb_get_config_descriptor(device_p,
                configuration_id, byref(config))
            if result:
                raise libusb1.USBError, result
            append(config.contents)

    def __del__(self):
        libusb1.libusb_unref_device(self.device_p)
        if self.configuration_descriptor_list is not None:
            for config in self.configuration_descriptor_list:
                libusb1.libusb_free_config_descriptor(byref(config))

    def __str__(self):
        return 'Bus %03i Device %03i: ID %04x:%04x %s %s' % (
            self.getBusNumber(),
            self.getDeviceAddress(),
            self.getVendorID(),
            self.getProductID(),
            self.getManufacturer(),
            self.getProduct()
        )

    def reprConfigurations(self):
        out = StringIO()
        for config in self.configuration_descriptor_list:
            print >> out, 'Configuration %i: %s' % (config.bConfigurationValue,
                self._getASCIIStringDescriptor(config.iConfiguration))
            print >> out, '  Max Power: %i mA' % (config.MaxPower * 2, )
            # TODO: bmAttributes dump
            for interface_num in xrange(config.bNumInterfaces):
                interface = config.interface[interface_num]
                print >> out, '  Interface %i' % (interface_num, )
                for alt_setting_num in xrange(interface.num_altsetting):
                    altsetting = interface.altsetting[alt_setting_num]
                    print >> out, '    Alt Setting %i: %s' % (alt_setting_num,
                        self._getASCIIStringDescriptor(altsetting.iInterface))
                    print >> out, '      Class: %02x Subclass: %02x' % \
                        (altsetting.bInterfaceClass,
                         altsetting.bInterfaceSubClass)
                    print >> out, '      Protocol: %02x' % \
                        (altsetting.bInterfaceProtocol, )
                    for endpoint_num in xrange(altsetting.bNumEndpoints):
                        endpoint = altsetting.endpoint[endpoint_num]
                        print >> out, '      Endpoint %i' % (endpoint_num, )
                        print >> out, '        Address: %02x' % \
                            (endpoint.bEndpointAddress, )
                        attribute_list = []
                        transfer_type = endpoint.bmAttributes & \
                            libusb1.LIBUSB_TRANSFER_TYPE_MASK
                        attribute_list.append(libusb1.libusb_transfer_type(
                            transfer_type
                        ))
                        if transfer_type == \
                            libusb1.LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
                            attribute_list.append(libusb1.libusb_iso_sync_type(
                                (endpoint.bmAttributes & \
                                 libusb1.LIBUSB_ISO_SYNC_TYPE_MASK) >> 2
                            ))
                            attribute_list.append(libusb1.libusb_iso_usage_type(
                                (endpoint.bmAttributes & \
                                 libusb1.LIBUSB_ISO_USAGE_TYPE_MASK) >> 4
                            ))
                        print >> out, '        Attributes: %s' % \
                            (', '.join(attribute_list), )
                        print >> out, '        Max Packet Size: %i' % \
                            (endpoint.wMaxPacketSize, )
                        print >> out, '        Interval: %i' % \
                            (endpoint.bInterval, )
                        print >> out, '        Refresh: %i' % \
                            (endpoint.bRefresh, )
                        print >> out, '        Sync Address: %02x' % \
                            (endpoint.bSynchAddress, )
        return out.getvalue()

    def getBusNumber(self):
        return libusb1.libusb_get_bus_number(self.device_p)

    def getDeviceAddress(self):
        return libusb1.libusb_get_device_address(self.device_p)

    def getbcdUSB(self):
        return self.device_descriptor.bcdUSB

    def getDeviceClass(self):
        return self.device_descriptor.bDeviceClass

    def getDeviceSubClass(self):
        return self.device_descriptor.bDeviceSubClass

    def getDeviceProtocol(self):
        return self.device_descriptor.bDeviceProtocol

    def getMaxPacketSize0(self):
        return self.device_descriptor.bMaxPacketSize0

    def getVendorID(self):
        return self.device_descriptor.idVendor

    def getProductID(self):
        return self.device_descriptor.idProduct

    def getbcdDevice(self):
        return self.device_descriptor.bcdDevice

    def _getASCIIStringDescriptor(self, descriptor):
        temp_handle = self.open()
        return temp_handle.getASCIIStringDescriptor(descriptor)

    def getManufacturer(self):
        return self._getASCIIStringDescriptor(
            self.device_descriptor.iManufacturer)

    def getProduct(self):
        return self._getASCIIStringDescriptor(self.device_descriptor.iProduct)

    def getSerialNumber(self):
        return self.device_descriptor.iSerialNumber

    def getNumConfigurations(self):
        return self.device_descriptor.bNumConfigurations

    def open(self):
        handle = libusb1.libusb_device_handle_p()
        result = libusb1.libusb_open(self.device_p, byref(handle))
        if result:
            raise libusb1.USBError, result
        return USBDeviceHandle(handle)

class LibUSBContext(object):

    context_p = None

    def __init__(self):
        context_p = libusb1.libusb_context_p()
        result = libusb1.libusb_init(byref(context_p))
        if result:
            raise libusb1.USBError, result
        self.context_p = context_p

    def __del__(self):
        if self.context_p is not None:
            libusb1.libusb_exit(self.context_p)

    def getDeviceList(self):
        device_p_p = libusb1.libusb_device_p_p()
        device_list_len = libusb1.libusb_get_device_list(self.context_p,
                                                         byref(device_p_p))
        result = [USBDevice(x) for x in device_p_p[:device_list_len]]
        # XXX: causes problems, why ?
        #libusb1.libusb_free_device_list(device_p_p, 1)
        return result

    def openByVendorIDAndProductID(self, vendor_id, product_id):
        handle_p = libusb1.libusb_open_device_with_vid_pid(self.context_p,
            vendor_id, product_id)
        if handle_p:
            result = USBDeviceHandle(handle_p)
        else:
            result = None
        return result

