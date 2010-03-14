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
