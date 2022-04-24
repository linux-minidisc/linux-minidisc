#include <QtDBus/QtDBus>
#include <qhimddetection.h>
#include <QVariantList>
#include <QDebug>
#include <QApplication>

class QHiMDLinuxDetection : public QHiMDDetection{
    Q_OBJECT

    QDBusConnection dbus_sys;

public:
    QHiMDLinuxDetection(QObject * parent = NULL);
    ~QHiMDLinuxDetection() {}
    virtual QString mountpoint(QMDDevice *dev);
    virtual void scan_for_himd_devices();

private:
    QVariant get_property(QString udiskPath, QString property, QString interface);
    bool drive_is_himd(QString udisk_drive_path, QString &name);
    bool udisks_drive_path(char drive, QString &drive_path);
    bool check_serial(QString udisk_drive_path, QString serial);
    QString deviceFile_from_serial(QString serial);
    QMDDevice *find_by_deviceFile(QString file);
    virtual void add_himddevice(QString path, QString name, libusb_device * dev);
};
