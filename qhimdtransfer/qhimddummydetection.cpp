#include <qhimddetection.h>

QHiMDDetection * createDetection(QObject * parent)
{
    return new QHiMDDetection(parent);
}
