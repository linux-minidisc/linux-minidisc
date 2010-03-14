#include <QDebug>
#include "qhimddetection.h"
#include "ui_qhimddetection.h"

QHiMDDetection::QHiMDDetection(QWidget *parent) :
    QDialog(parent)
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
