#ifndef QHIMDDETECTION_H
#define QHIMDDETECTION_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QList>
#include <QString>
#include <qmddevice.h>
#include "libusb.h"

typedef QList<QMDDevice *> QMDDevicePtrList;

// polling object for libusb hotplug events
class QLibusbPoller : public QThread {
    Q_OBJECT
    Q_DISABLE_COPY(QLibusbPoller)

protected:
    QTimer t;
    libusb_context *lct;
    virtual void run();
protected slots:
    void poll();
public:
    QLibusbPoller(QObject *parent = 0, libusb_context *ctx = 0);
    virtual ~QLibusbPoller();
    void idle();
    void continue_polling();
    /* provide static sleep function */
    static void sleep(unsigned long secs) {QThread::sleep(secs);}
};


class QHiMDDetection : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(QHiMDDetection)

protected:
    QMDDevicePtrList dlist;
    netmd_device * dev_list;
    libusb_hotplug_callback_handle cb_handle;
    QLibusbPoller * poller;
    libusb_context * ctx;
public:
    explicit QHiMDDetection(QObject *parent = 0);
    virtual ~QHiMDDetection();
    bool start_hotplug();
    virtual void clearDeviceList();
    void rescan_netmd_devices();
    void scan_for_minidisc_devices();
    virtual void scan_for_himd_devices() {}      // platform dependent
    virtual void add_hotplug_device(libusb_device * dev);
    virtual void remove_hotplug_device(libusb_device * dev);
    void scan_for_netmd_devices();
    QMDDevice *find_by_path(QString path);
    QMDDevice *find_by_name(QString name);
    QMDDevice *find_by_libusbDevice(libusb_device * dev);
    virtual QString mountpoint(QMDDevice *dev);
    QString getDefaultLabel();
private:
    virtual void add_himddevice(QString path, QString name, libusb_device * dev) {}
    virtual void remove_himddevice(QString path, libusb_device * dev = NULL);
    void add_netmddevice(libusb_device * dev, QString name);
    void remove_netmddevice(libusb_device * dev);

signals:
    void deviceListChanged(QMDDevicePtrList list);
};

QHiMDDetection * createDetection(QObject * parent = NULL);

#endif // QHIMDDETECTION_H
