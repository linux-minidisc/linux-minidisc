#include <QtCore/QDebug>
#include "qhimddetection.h"
#include "ui_qhimddetection.h"

QHiMDDetection::QHiMDDetection(QObject *parent) :
    QObject(parent)
{
}

himd_device *QHiMDDetection::find_by_path(QString path)
{
    for (int i = 0; i < device_list.size(); i++)
        if(device_list.at(i)->path == path)
            return device_list.at(i);

    return NULL;
}

// slots

void QHiMDDetection::himd_busy(QString path)
{
    himd_device * dev = find_by_path(path);
    if (!dev)
        return;

    dev->is_busy = true;
    qDebug() << "himd device at " + dev->path + " : device busy, starting transfer";
}

void QHiMDDetection::himd_idle(QString path)
{
    himd_device * dev = find_by_path(path);
    if (!dev)
        return;

    dev->is_busy = false;
    qDebug() << "himd device at " + dev->path + " : device idle, transfer complete";
}

const char * identify_usb_device(int vid, int pid)
{
    if (vid != SONY)
        return NULL;

    switch (pid)
    {
        case MZ_NH1:
            return "SONY MZ-NH1";
        case MZ_NH3D:
            return "SONY MZ-NH3D";
        case MZ_NH900:
            return "SONY MZ-NH900";
        case MZ_NH700:
            return "SONY MZ-NH700 / MZ-NH800";
        case MZ_NH600:
            return "SONY MZ-NH600(D)";
        case LAM_3:
            return "SONY LAM-3";
        case MZ_DH10P:
            return "SONY MZ-DH10P";
        case MZ_RH10:
            return "SONY MZ-RH10";
        case MZ_RH910:
            return "SONY MZ-RH910";
        case CMT_AH10:
            return "SONY CMT-AH10";
        case DS_HMD1:
            return "SONY DS-HMD1";
        case MZ_RH1:
            return "SONY MZ-RH1";
    }
    return NULL;
}
