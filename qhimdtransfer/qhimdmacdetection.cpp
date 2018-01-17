#include "qhimddetection.h"

#include <QList>
#include <QDir>
#include <QFile>

class QHiMDMacDetection : public QHiMDDetection {
public:
    QHiMDMacDetection(QObject * parent = NULL);
    ~QHiMDMacDetection();

    void scan_for_himd_devices();

private:
    virtual void add_himddevice(QString path, QString name, libusb_device * dev);
};


QHiMDDetection * createDetection(QObject * parent)
{
    return new QHiMDMacDetection(parent);
}

QHiMDMacDetection::QHiMDMacDetection(QObject * parent)
    : QHiMDDetection(parent)
{
    ctx = NULL;
    dev_list = NULL;
}

QHiMDMacDetection::~QHiMDMacDetection()
{
     poller->idle();
     poller->quit();
     delete poller;
     libusb_hotplug_deregister_callback(ctx, cb_handle);
     libusb_exit(ctx);
     clearDeviceList();
     if(!ctx)
         netmd_clean(&dev_list);
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
            add_himddevice(fullPath, path, NULL);
        }
    }
}

void QHiMDMacDetection::add_himddevice(QString path, QString name, libusb_device * dev = NULL)
{
    QMDDevice *device;

    if(dev) {
        /* already present in the device list */
        if(find_by_libusbDevice(dev))
            return;
         /* TODO: find path for the corresponding device
          * qhimdtransfer will ask for path if not provided */
    }
    else {
        foreach (device, dlist) {
            if (device->path() == path) {
                // Device is already added -- skip duplicate
                return;
            }
        }
    }

    QHiMDDevice * new_device = new QHiMDDevice();
    new_device->setMdInserted(true);
    new_device->setName(name);
    new_device->setPath(path);
    new_device->setBusy(false);
    new_device->setLibusbDevice(dev);

    dlist.append(new_device);
    emit deviceListChanged(dlist);
}
