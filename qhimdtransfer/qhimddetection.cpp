#include <QDebug>
#include "qhimddetection.h"

/* callback function for libusb hotplug events, void *user_data is a pointer to the running QHiMDDetection object */
static int hotplug_cb(struct libusb_context *ctx, struct libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    static libusb_device_handle *handle = NULL;
    struct libusb_device_descriptor desc;
    QString name;
    unsigned char serial[13];
    int rc = 0;
    QHiMDDetection * detect = static_cast<QHiMDDetection *>(user_data);

    memset(serial, 0, 13);
    libusb_get_device_descriptor(dev, &desc);
    name = QString(identify_usb_device(desc.idVendor, desc.idProduct));

    // return if usb device is not a minidisc device
    if(name.isEmpty())
        return 0;

    // handle netmd devices, reenumerate netmd device list
    if(name.contains("NetMD")) {
        if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED || event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
            qDebug() << QString("qhimddetection : hotplug event for %1 detected, rescan netmd devices").arg(name);
            detect->rescan_netmd_devices();
        }
        return 0;
    }

    // handle himd devices, use platform dependent code
    if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        rc = libusb_open(dev, &handle);
        if (rc != LIBUSB_SUCCESS)
            return 0;

        libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, serial, 13);
        qDebug() << QString("qhimddetection : hotplug event for %1 detected, serial no. %2").arg(name).arg(QString((const char *)serial));
        libusb_close(handle);

        // wait some time (linux: let udev and udisks2 finish processing)
        QLibusbPoller::sleep(4);
        detect->add_himddevice(QString(), name, QString((const char *)serial));
    }
     else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
        qDebug() << QString("qhimddetection : hotplug event for %1 detected. device removed").arg(name);
        detect->remove_himddevice(QString(), name);
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
            if(nmddev->isOpen())
                nmddev->close();
            delete nmddev;
            nmddev = NULL;
            dlist.removeAt(i);
            continue;
        }
        else if(mddev->deviceType() == HIMD_DEVICE)
        {
            remove_himddevice(mddev->path(), mddev->name());  // uses platform dependent function if available
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
}

QHiMDDetection::~QHiMDDetection()
{
    poller->idle();
    poller->quit();
    delete poller;
    libusb_hotplug_deregister_callback(ctx, cb_handle);
    libusb_exit(ctx);
    clearDeviceList();
    cleanup_netmd_list();
}

bool QHiMDDetection::start_hotplug()
{
    int reg_hotplug;

    libusb_init(&ctx);

    if(!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        qDebug() << tr("usb hotplug events not supported, autodetection disabled");
        return false;
    }

    reg_hotplug = libusb_hotplug_register_callback(ctx,
                                          (libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                                          (libusb_hotplug_flag)0,
                                          LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                          LIBUSB_HOTPLUG_MATCH_ANY, hotplug_cb, (void *)this,
                                          &cb_handle);
    if (reg_hotplug != LIBUSB_SUCCESS) {
        qDebug() << tr("Error creating usb hotplug callback, autodetection disabled");
        return false;
    }

    poller = new QLibusbPoller(this, ctx);
    poller->start();

    return true;
}

void QHiMDDetection::cleanup_netmd_list()
{
    if(dev_list != NULL)
        netmd_clean(&dev_list);
}

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
        dev = static_cast<QNetMDDevice *>(dlist.at(i));
        if(dev->isOpen())
            dev->close();

        delete dev;
        dev = NULL;
        dlist.removeAt(i);
    }

    netmd_clean(&dev_list);
    dev_list = NULL;

    emit deviceListChanged(dlist);
    scan_for_netmd_devices();
}

void QHiMDDetection::scan_for_minidisc_devices()
{
    /* create device entry for disc images first */
    QHiMDDevice * mddev = new QHiMDDevice();
    mddev->setMdInserted(false);
    mddev->setName("disc image");
    dlist.append(mddev);
    emit deviceListChanged(dlist);

    scan_for_himd_devices();
    scan_for_netmd_devices();
}

void QHiMDDetection::remove_himddevice(QString path, QString name)
{
    /* try to find device by name if path is not set, libusb hotplug event does not provide a path */
    QMDDevice * d = NULL;
    QHiMDDevice * dev = NULL;
    int i = path.isEmpty() ? dlist.indexOf(find_by_name(name)) : dlist.indexOf(find_by_path(path));
    int himd_count = 0;

    if(i >= 0)
        dev = static_cast<QHiMDDevice *>(dlist.at(i));

    /* remove himd device if device cannot be found (name or path not set correctly) but
     * only one real himd device is stored in the device list */
    if(!dev) {
        foreach(d, dlist) {
            if(d->deviceType() == HIMD_DEVICE && !d->name().contains("disc image")) {
                himd_count++;
                i = dlist.indexOf(d);
            }
        }
        if(himd_count != 1)
            return;
        dev = static_cast<QHiMDDevice *>(dlist.at(i));
    }

    if(!dev)
        return;

    if(dev->isOpen())
        dev->close();
    delete dev;
    dev = NULL;

    dlist.removeAt(i);
    emit deviceListChanged(dlist);
}

void QHiMDDetection::scan_for_netmd_devices()
{
    netmd_device * md;
    netmd_error error = netmd_init(&dev_list);
    struct libusb_device_descriptor desc;
    QNetMDDevice * mddev;

    if (error != NETMD_NO_ERROR)
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

const char * identify_usb_device(int vid, int pid)
{
    if(vid == SHARP)
    {
        switch(pid)
            {
            case IM_MT880H:
                return "SHARP IM-MT880H / IM-MT899H (NetMD)";
            case IM_DR400:
                return "SHARP IM-DR400 / IM-DR410 (NetMD)";
            case IM_DR80:
                return "SHARP IM-DR80 / IM-DR420/ IM-DR580 or KENWOOD DMC-S9NET (NetMD)";
            }
    }

    if (vid != SONY)
        return NULL;

    switch (pid)
    {
        case MZ_NH1_HIMD:
            return "SONY MZ-NH1";
        case MZ_NH3D_HIMD:
            return "SONY MZ-NH3D";
        case MZ_NH900_HIMD:
            return "SONY MZ-NH900";
        case MZ_NH700_HIMD:
            return "SONY MZ-NH700 / MZ-NH800";
        case MZ_NH600_HIMD:
            return "SONY MZ-NH600(D)";
        case LAM_3_HIMD:
            return "SONY LAM-3";
        case MZ_DH10P_HIMD:
            return "SONY MZ-DH10P";
        case MZ_RH10_HIMD:
            return "SONY MZ-RH10";
        case MZ_RH910_HIMD:
            return "SONY MZ-RH910";
        case CMT_AH10_HIMD:
            return "SONY CMT-AH10";
        case DS_HMD1_HIMD:
            return "SONY DS-HMD1";
        case MZ_RH1_HIMD:
            return "SONY MZ-RH1";
        case PCLK_XX:
            return "SONY PCLK-XX (NetMD)";
        case UNKNOWN_A:
            return "SONY (unknown model, NetMD)";
        case MZ_N1:
            return "SONY MZ-N1 (NetMD)";
        case UNKNOWN_B:
            return "SONY (unknown model, NetMD)";
        case LAM_1:
            return "Sony LAM-1 (NetMD)";
        case MDS_JE780:
            return "SONY MDS-JE780 / MDS-JE980 (NetMD)";
        case MZ_N505:
            return "SONY MZ-N505 (NetMD)";
        case MZ_S1:
            return "SONY MZ-S1 (NetMD)";
        case MZ_N707:
            return "SONY MZ-N707 (NetMD)";
        case CMT_C7NT:
            return "SONY CMT-C7NT (NetMD)";
        case PCGA_MDN1:
            return "SONY PCGA-MDN1 (NetMD)";
        case CMT_L7HD:
            return "SONY CMT-L7HD (NetMD)";
        case MZ_N10:
            return "SONY MZ-N10 (NetMD)";
        case MZ_N910:
            return "SONY MZ-N910 (NetMD)";
        case MZ_N710:
            return "SONY MZ-N710 / MZ-NE810 / MZ-NF810 (NetMD)";
        case MZ_N510:
            return "SONY MZ-N510 (NetMD)";
        case MZ_NE410:
            return "SONY MZ-NE410 / MZ-DN430 / MZ-NF520 (NetMD)";
        case MZ_NE810:
            return "SONY MZ-NE810 / MZ-NE910 (NetMD)";
        case CMT_M333NT:
            return "SONY CMT-M333NT / CMT_M373NT (NetMD)";
        case LAM_10:
            return "SONY LAM-10 (NetMD)";
        case AIWA_AM_NX1:
            return "AIWA AM-NX1 (NetMD)";
        case AIWA_AM_NX9:
            return "AIWA AM-NX9 (NetMD)";
        case MZ_NH1:
            return "SONY MZ-NH1 (NetMD)";
        case MZ_NH3D:
            return "SONY MZ-NH3D (NetMD)";
        case MZ_NH900:
            return "SONY MZ-NH900 (NetMD)";
        case MZ_NH700:
            return "SONY MZ-NH700 / MZ-NH800 (NetMD)";
        case MZ_NH600:
            return "SONY MZ-NH600 / MZ-NH600D (NetMD)";
        case MZ_N920:
            return "SONY MZ-N920 (NetMD)";
        case LAM_3:
            return "SONY LAM-3 (NetMD)";
        case MZ_DH10P:
            return "SONY MZ-DH10P (NetMD)";
        case MZ_RH10:
            return "SONY MZ-RH10 (NetMD)";
        case MZ_RH910:
            return "SONY MZ-RH910 (NetMD)";
        case CMT_AH10_A:
            return "SONY CMT-AH10 (NetMD)";
        case CMT_AH10_B:
            return "SONY CMT-AH10 (NetMD)";
        case DS_HMD1:
            return "SONY DS-HMD1 (NetMD)";
        case MZ_RH1:
            return "SONY MZ-RH1 (NetMD)";
    }
    return NULL;
}
