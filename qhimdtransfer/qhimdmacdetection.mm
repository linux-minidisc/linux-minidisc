#include "qhimddetection.h"

#include <QList>
#include <QDir>
#include <QFile>

#include "libusbmd.h"

#import <DiskArbitration/DiskArbitration.h>
#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>

class QHiMDMacDetection : public QHiMDDetection {
public:
    QHiMDMacDetection(QObject * parent = NULL);
    ~QHiMDMacDetection();

    void scan_for_himd_devices();

    void on_mountpoint_known(int pid, int vid, const QString &path);

private:
    DASessionRef session;
};

// https://developer.apple.com/forums/thread/121557
static bool
getUSBVendorProductID(DADiskRef dsk, int *vid, int *pid)
{
    io_service_t ioService = DADiskCopyIOMedia(dsk);
    if (!ioService) {
        return false;
    }

    int result = true;

    CFTypeRef vendorid = IORegistryEntrySearchCFProperty(ioService, kIOServicePlane, CFSTR("idVendor"), NULL, kIORegistryIterateParents | kIORegistryIterateRecursively);
    if(vendorid && CFGetTypeID(vendorid) == CFNumberGetTypeID()) {
        if (!CFNumberGetValue((CFNumberRef)vendorid, kCFNumberIntType, vid)) {
            result = false;
        }

        CFRelease(vendorid);
    } else {
        result = false;
    }

    CFTypeRef productid = IORegistryEntrySearchCFProperty(ioService, kIOServicePlane, CFSTR("idProduct"), NULL, kIORegistryIterateParents | kIORegistryIterateRecursively);
    if(productid && CFGetTypeID(productid) == CFNumberGetTypeID()) {
        if (!CFNumberGetValue((CFNumberRef)productid, kCFNumberIntType, pid)) {
            result = false;
        }

        CFRelease(productid);
    } else {
        result = false;
    }

    IOObjectRelease(ioService);

    return result;
}

// https://gist.github.com/boredzo/5346677
static void
on_disk_appeared(DADiskRef disk, void *context)
{
    QHiMDMacDetection *detection = static_cast<QHiMDMacDetection *>(context);

    // https://stackoverflow.com/a/17419866/1047040
    // FIXME: Potentially some memory leak here
    NSDictionary *description = CFBridgingRelease(DADiskCopyDescription(disk));

    int vid = 0, pid = 0;
    if (getUSBVendorProductID(disk, &vid, &pid)) {
        // FIXME: Does it return a ref or a new object we have to release?
        NSURL *url = [description objectForKey:@"DAVolumePath"];
        if (url != NULL) {
            NSString *path = [url path];
            detection->on_mountpoint_known(vid, pid, [path UTF8String]);
        }
    }
}

QHiMDDetection * createDetection(QObject * parent)
{
    return new QHiMDMacDetection(parent);
}

QHiMDMacDetection::QHiMDMacDetection(QObject * parent)
    : QHiMDDetection(parent)
    , session(DASessionCreate(kCFAllocatorDefault))
{
    DASessionSetDispatchQueue(session, dispatch_get_main_queue());
    DARegisterDiskAppearedCallback(session, kDADiskDescriptionMatchVolumeMountable, on_disk_appeared, this);
}

QHiMDMacDetection::~QHiMDMacDetection()
{
    DAUnregisterCallback(session, (void *)on_disk_appeared, this);
    DASessionSetDispatchQueue(session, nullptr);
    CFRelease(session);
}

void QHiMDMacDetection::scan_for_himd_devices()
{
    const QString BASE_DIR = "/Volumes/";

    /* skip enumeration when libusb hotplug events are used
     * path is empty for hotplugged devices but will be asked for and set when opening
     * alternatively use enueration but hotplug events will not detect disconnection of these devices */
    if(ctx)
        return;

    QString path;
    foreach (path, QDir(BASE_DIR).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString fullPath = BASE_DIR + path;
        if (QFile(fullPath + "/HI-MD.IND").exists()) {
            QHiMDDevice * new_device = new QHiMDDevice();
            new_device->setMdInserted(true);
            new_device->setName(path);
            new_device->setPath(fullPath);
            new_device->setBusy(false);
            new_device->setLibusbDevice(nullptr);

            dlist.append(new_device);
            emit deviceListChanged(dlist);
        }
    }
}

void
QHiMDMacDetection::on_mountpoint_known(int vid, int pid, const QString &path)
{
    for (auto &device: dlist) {
        if (device->path() == path) {
            // Already known path, ignore
            return;
        }
    }

    auto info = minidisc_usb_device_info_get(vid, pid);

    if (info != nullptr) {
        QHiMDDevice * new_device = new QHiMDDevice();
        new_device->setMdInserted(true);
        new_device->setName(info->name);
        new_device->setPath(path);
        new_device->setBusy(false);
        new_device->setLibusbDevice(nullptr);

        dlist.append(new_device);
        emit deviceListChanged(dlist);
    }
}
