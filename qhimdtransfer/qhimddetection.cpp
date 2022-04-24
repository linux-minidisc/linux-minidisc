#include <QDebug>
#include "qhimddetection.h"
#include "libusbmd.h"

static const char *
identify_usb_device(int vid, int pid)
{
    auto info = minidisc_usb_device_info_get(vid, pid);
    return info ? info->name : NULL;
}

/* callback function for libusb hotplug events, void *user_data is a pointer to the running QHiMDDetection object */
static int LIBUSB_CALL hotplug_cb(struct libusb_context *ctx, struct libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    struct libusb_device_descriptor desc;
    QString name;
    QHiMDDetection * detect = static_cast<QHiMDDetection *>(user_data);

    libusb_get_device_descriptor(dev, &desc);
    name = QString(identify_usb_device(desc.idVendor, desc.idProduct));

    // return if usb device is not a minidisc device
    if(name.isEmpty())
        return 0;

    if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        qDebug() << QString("qhimddetection: hotplug event for %1, device connected").arg(name);
        detect->add_hotplug_device(dev);
    }
    else if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
        qDebug() << QString("qhimddetection: hotplug event for %1, device removed").arg(name);
        detect->remove_hotplug_device(dev);
    }

    return 0;
}

QLibusbPoller::QLibusbPoller(QObject *parent, libusb_context *ctx) : QThread(parent), lct(ctx)
{
}

QLibusbPoller::~QLibusbPoller()
{
    t.stop();
    wait();
}

void QLibusbPoller::run()
{
    connect( &t, SIGNAL( timeout() ), this, SLOT( poll() ) );
    t.start( 1000 );
    exec();
}

void QLibusbPoller::poll()
{
    // use non-blocking libusb routine
    struct timeval tv = { 0 , 0 };
    libusb_handle_events_timeout_completed(lct, &tv, NULL);
}

void QLibusbPoller::idle()
{
    t.stop();
}

void QLibusbPoller::continue_polling()
{
    t.start( 1000 );
}


QString QHiMDDetection::mountpoint(QMDDevice *dev)
{
    return dev->path();
}

void QHiMDDetection::clearDeviceList()
{
    QMDDevice * mddev;
    QNetMDDevice * nmddev;
    int i = 0;

    while( i < dlist.count() )
    {
        mddev = dlist.at(i);
        if(mddev->deviceType() == NETMD_DEVICE)
        {
            nmddev = static_cast<QNetMDDevice *>(mddev);
            remove_netmddevice(nmddev->libusbDevice());
            continue;
        }
        else if(mddev->deviceType() == HIMD_DEVICE)
        {
            remove_himddevice(mddev->path(), mddev->libusbDevice());  // use platform dependent function if available
            continue;
        }
    }

    if(!dlist.isEmpty())
        dlist.clear();
    emit deviceListChanged(dlist);
}


QHiMDDetection::QHiMDDetection(QObject *parent) :
    QObject(parent)
{
    ctx = NULL;
    dev_list = NULL;
}

QHiMDDetection::~QHiMDDetection()
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

bool QHiMDDetection::start_hotplug()
{
    int reg_hotplug;

    libusb_init(&ctx);

    /* create device entry for disc images first, should be the first entry */
    QHiMDDevice * mddev = new QHiMDDevice();
    mddev->setMdInserted(false);
    mddev->setName("disc image");
    dlist.append(mddev);
    emit deviceListChanged(dlist);

    if(!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        qDebug() << tr("usb hotplug events not supported, autodetection disabled");
        libusb_exit(ctx);
        ctx = NULL;
        return false;
    }

    reg_hotplug = libusb_hotplug_register_callback(ctx,
                                          (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                                          LIBUSB_HOTPLUG_ENUMERATE,
                                          LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                          LIBUSB_HOTPLUG_MATCH_ANY, hotplug_cb, (void *)this,
                                          &cb_handle);
    if (reg_hotplug != LIBUSB_SUCCESS) {
        qDebug() << tr("Error creating usb hotplug callback, autodetection disabled");
        libusb_exit(ctx);
        ctx = NULL;
        return false;
    }

    poller = new QLibusbPoller(this, ctx);
    poller->start();

    return true;
}

/* only needed if hotplug support is disabled */
void QHiMDDetection::rescan_netmd_devices()
{
    QNetMDDevice * dev;
    int i = 0;

    // find and remove netmd devices
    while(i < dlist.count())
    {
        if(dlist.at(i)->deviceType() != NETMD_DEVICE)
        {
            i++;
            continue;
        }
        remove_netmddevice(dlist.at(i)->libusbDevice());
    }

    netmd_clean(&dev_list);
    dev_list = NULL;

    scan_for_netmd_devices();
}

void QHiMDDetection::scan_for_minidisc_devices()
{
    scan_for_himd_devices();
    scan_for_netmd_devices();
}

void QHiMDDetection::add_hotplug_device(libusb_device * dev)
{
    struct libusb_device_descriptor desc;
    QString name;

    libusb_get_device_descriptor(dev, &desc);

    auto info = minidisc_usb_device_info_get(desc.idVendor, desc.idProduct);

    if (info != nullptr) {
        name = info->name;

        if (info->device_type == MINIDISC_USB_DEVICE_TYPE_NETMD) {
            add_netmddevice(dev, name);
        } else {
            /* wait some time (linux: let udev and udisks2 finish processing) */
            QLibusbPoller::sleep(4);
            add_himddevice(QString(), name, dev);
        }
    }
}

void QHiMDDetection::remove_hotplug_device(libusb_device * dev)
{
    struct libusb_device_descriptor desc;

    libusb_get_device_descriptor(dev, &desc);

    auto info = minidisc_usb_device_info_get(desc.idVendor, desc.idProduct);

    if (info != nullptr) {
        if (info->device_type == MINIDISC_USB_DEVICE_TYPE_NETMD) {
            remove_netmddevice(dev);
        } else {
            remove_himddevice(QString(), dev);
        }
    }
}

/* use this if no platform dependent implementation is available,
 * just handle libusb hotplug remove events, removing by path is platform dependent */
void QHiMDDetection::remove_himddevice(QString path, libusb_device * dev)
{
    QHiMDDevice * hdev = NULL;
    int i = dlist.indexOf(find_by_libusbDevice(dev));

    if(i < 0)
        return;

    hdev = static_cast<QHiMDDevice *>(dlist.at(i));

    if(!hdev)
        return;

    if(hdev->isOpen())
        hdev->close();
    delete hdev;
    hdev = NULL;

    dlist.removeAt(i);
    emit deviceListChanged(dlist);
}

void QHiMDDetection::add_netmddevice(libusb_device * dev, QString name)
{
    QNetMDDevice * mddev;
    netmd_device * new_dev;

    /* skip duplicate if device alredy exists in the device list*/
    if(find_by_libusbDevice(dev))
        return;

    mddev = new QNetMDDevice();
    new_dev = static_cast<netmd_device *>(malloc(sizeof(netmd_device)));
    new_dev->usb_dev = dev;
    new_dev->link = NULL;
    mddev->setName(name);
    mddev->setUsbDevice(new_dev);
    mddev->setLibusbDevice(dev);

    dlist.append(mddev);
    emit deviceListChanged(dlist);
}

void QHiMDDetection::remove_netmddevice(libusb_device * dev)
{
    QMDDevice * dd = find_by_libusbDevice(dev);
    QNetMDDevice * d;
    int index;

    if(!dd)
        return;

    index = dlist.indexOf(dd);
    d = static_cast<QNetMDDevice *>(dd);

    if(d->isOpen())
        d->close();

    free(d->usbDevice());
    delete d;
    d = NULL;

    if(index >= 0)
        dlist.removeAt(index);

    emit deviceListChanged(dlist);
}

void QHiMDDetection::scan_for_netmd_devices()
{
    netmd_device * md;
    netmd_error error = netmd_init(&dev_list, ctx);
    struct libusb_device_descriptor desc;
    QNetMDDevice * mddev;

    /* skip enumeration when using libusb hotplug feature
     * else use device enumeration initialized by netmd_init() */
    if (error  == NETMD_USE_HOTPLUG || error  != NETMD_NO_ERROR)
        return;

    md = dev_list;  // pick first device

    while( md != NULL) {
        libusb_get_device_descriptor(md->usb_dev, &desc);
        mddev = new QNetMDDevice();
        mddev->setName(identify_usb_device(desc.idVendor, desc.idProduct));
        mddev->setUsbDevice(md);
        dlist.append(mddev);
        emit deviceListChanged(dlist);
        md = md->link;  // pick next device
    }
}

QMDDevice *QHiMDDetection::find_by_path(QString path)
{
    QMDDevice * mddev;

    foreach(mddev, dlist)
    {
        if(mddev->path() == path)
            return mddev;
    }
    return NULL;
}

QMDDevice *QHiMDDetection::find_by_name(QString name)
{
    QMDDevice * mddev;

    foreach(mddev, dlist)
    {
        if(mddev->name() == name)
            return mddev;
    }
    return NULL;
}

QMDDevice *QHiMDDetection::find_by_libusbDevice(libusb_device * dev)
{
    QMDDevice * mddev;

    foreach(mddev, dlist)
    {
        if(mddev->libusbDevice() == dev)
            return mddev;
    }
    return NULL;
}
