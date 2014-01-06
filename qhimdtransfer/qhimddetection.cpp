#include <QDebug>
#include "qhimddetection.h"

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
            remove_himddevice(mddev->path());  // uses platform dependent function if available
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
    clearDeviceList();
    cleanup_netmd_list();
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
    mddev->setMdInserted(true);
    mddev->setName("disc image");
    dlist.append(mddev);
    emit deviceListChanged(dlist);

    scan_for_himd_devices();
    scan_for_netmd_devices();
}

void QHiMDDetection::remove_himddevice(QString path)
{
    QHiMDDevice * dev = static_cast<QHiMDDevice *>(find_by_path(path));
    int i = dlist.indexOf(find_by_path(path));

    if(i < 0)
        return;

    if(dev->isOpen())
        dev->close();
    delete dev;
    dev = NULL;

    dlist.removeAt(i);
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
