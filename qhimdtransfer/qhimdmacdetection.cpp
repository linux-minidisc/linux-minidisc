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
    void add_himddevice(QString path, QString name);
};


QHiMDDetection * createDetection(QObject * parent)
{
    return new QHiMDMacDetection(parent);
}

QHiMDMacDetection::QHiMDMacDetection(QObject * parent)
    : QHiMDDetection(parent)
{
}

QHiMDMacDetection::~QHiMDMacDetection()
{
}

void QHiMDMacDetection::scan_for_himd_devices()
{
    const QString BASE_DIR = "/Volumes/";

    QString path;
    foreach (path, QDir(BASE_DIR).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString fullPath = BASE_DIR + path;
        if (QFile(fullPath + "/HI-MD.IND").exists()) {
            add_himddevice(fullPath, path);
        }
    }
}

void QHiMDMacDetection::add_himddevice(QString path, QString name)
{
    QMDDevice *device;
    foreach (device, dlist) {
        if (device->path() == path) {
            // Device is already added -- skip duplicate
            return;
        }
    }

    QHiMDDevice * new_device = new QHiMDDevice();
    new_device->setMdInserted(true);
    new_device->setName(name);
    new_device->setPath(path);
    new_device->setBusy(false);

    dlist.append(new_device);
    emit deviceListChanged(dlist);
}
