#include <QtDBus/QtDBus>
#include <qhimddetection.h>
#include <QVariantList>
#include <QDebug>
#include <QApplication>

/* define some constants for UDisks2 bdus operations */
#define UDISK_SERVICE "org.freedesktop.UDisks2"
#define UDISK_PATH "/org/freedesktop/UDisks2"
#define UDISK_INTERFACE "org.freedesktop.UDisks2"
#define UDISK_PROPERTIES "org.freedesktop.DBus.Properties"
#define UDISK_INTERFACE_FILESYSTEM "org.freedesktop.UDisks2.Filesystem"
#define UDISK_INTERFACE_BLOCK "org.freedesktop.UDisks2.Block"
#define UDISK_INTERFACE_DRIVE "org.freedesktop.UDisks2.Drive"
#define UDISK_DEVICE_PATH "/org/freedesktop/UDisks2/block_devices/"


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

/* force qmake to run moc on this .cpp file */
#include "qhimdlinuxdetection.moc"
//  MOC_SKIP_BEGIN

QHiMDDetection * createDetection(QObject * parent)
{
    return new QHiMDLinuxDetection(parent);
}

/* return value for "getProperty" "Mountpoints" is an array of QByteArrays,
 * following static functions are used to extract and convert dbus replies to a QstringList,
 * */
static bool argToString(const QDBusArgument &busArg, QStringList &out);

static bool variantToString(const QVariant &arg, QStringList &out)
{
    int argType = arg.userType();

    if (argType == QVariant::ByteArray)
    {
        out.append(QString(arg.toByteArray()));
    }
    else if (argType == QVariant::String)
    {
        out.append(arg.toString());
    }
    else if (argType == QVariant::StringList)
    {
        out = arg.toStringList();
    }
    else if (argType == qMetaTypeId<QDBusArgument>())
     {
        argToString(qvariant_cast<QDBusArgument>(arg), out);
     }
    else if (argType == qMetaTypeId<QDBusVariant>())
    {
        const QVariant v = qvariant_cast<QDBusVariant>(arg).variant();

        if (!variantToString(v, out))
            return false;
    }
    else if (argType == qMetaTypeId<QDBusObjectPath>())
     {
        QDBusObjectPath p = qvariant_cast<QDBusObjectPath>(arg);
        out.append(p.path());
     }
    return true;
}

static bool argToString(const QDBusArgument &busArg, QStringList &out)
{
    bool doIterate = false;
    QDBusArgument::ElementType elementType = busArg.currentType();

    switch (elementType)
    {
        case QDBusArgument::BasicType:
        case QDBusArgument::VariantType:
            if (!variantToString(busArg.asVariant(), out))
                return false;
            break;
        case QDBusArgument::ArrayType:
            busArg.beginArray();
            doIterate = true;
            break;
        case QDBusArgument::UnknownType:
        default:
            return false;
    }

    if (doIterate && !busArg.atEnd()) {
        while (!busArg.atEnd()) {
            if (!argToString(busArg, out))
                return false;
        }
    }

    if(elementType == QDBusArgument::ArrayType)
        busArg.endArray();
    return true;
}

QHiMDLinuxDetection::QHiMDLinuxDetection(QObject *parent)
  : QHiMDDetection(parent), dbus_sys(QDBusConnection::connectToBus( QDBusConnection::SystemBus, "system" ) )
{
    if(!dbus_sys.isConnected())
        qDebug() << tr("dbus error, cannot connect to system bus");
    ctx = NULL;
    dev_list = NULL;
}

QVariant QHiMDLinuxDetection::get_property(QString udiskPath, QString property, QString interface)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(UDISK_SERVICE, udiskPath, UDISK_PROPERTIES, "Get");
    QList<QVariant> args;
    QDBusMessage reply;

    /* set arguments */
    args.append(interface);
    args.append(property);
    msg.setArguments(args);

    /* send message */
    reply = dbus_sys.call(msg);

    if (!reply.signature().compare(QString(QChar('v'))) && reply.arguments().length() >0)
        return reply.arguments().at(0);
    else
        return QVariant();
}

bool QHiMDLinuxDetection::drive_is_himd(QString udisk_drive_path, QString &name)
{
    QVariant ret;
    QStringList reply, vendor;

    /* check if connection bus is usb */
    ret = get_property(udisk_drive_path, "ConnectionBus", UDISK_INTERFACE_DRIVE);
    if(!ret.isValid())
        return false;
    if(!variantToString(ret, reply))
        return false;
    if(reply.at(0).compare("usb", Qt::CaseInsensitive))
        return false;

    ret.clear();
    reply.clear();

    /* check if vendor is "SONY" and Model is "Hi-MD"
    * cannot get usb VendorID/ProductID from udisks */
    ret = get_property(udisk_drive_path, "Vendor", UDISK_INTERFACE_DRIVE);
    if(!ret.isValid())
        return false;
    if(!variantToString(ret, reply))
        return false;
    if(reply.at(0).compare("Sony", Qt::CaseInsensitive))
        return false;

    vendor << reply.at(0);
    ret.clear();
    reply.clear();

    ret = get_property(udisk_drive_path, "Model", UDISK_INTERFACE_DRIVE);
    if(!ret.isValid())
        return false;
    if(!variantToString(ret, reply))
        return false;
    if(reply.at(0).compare("Hi-MD", Qt::CaseInsensitive))
        return false;

    vendor << reply.at(0);
    name = vendor.join(" ");
    return true;
}

bool QHiMDLinuxDetection::udisks_drive_path(char drive, QString &drive_path)
{
    QString device = QString("/org/freedesktop/UDisks2/block_devices/sd").append(drive);
    QVariant ret;
    QStringList str;

    /* check if device /dev/sd* is a valid udisk drive, ret is invalid if device not existst */
    ret = get_property(device, "Drive",  UDISK_INTERFACE_BLOCK);
    if(!ret.isValid())
        return false;

    /* return true if device is valid, but clear drive_path if it is not a valid udisks drive */
    if(!variantToString(ret, str)) {
        drive_path.clear();
    }
    else
        drive_path = str.at(0);

    return true;
}

QString QHiMDLinuxDetection::mountpoint(QMDDevice *dev)
{
    QString devpath = dev->deviceFile();
    QString udisk_path = UDISK_DEVICE_PATH;
    QVariant ret;
    QStringList mp;

    /* return mountpoint if it is already known */
    if(!dev->path().isEmpty())
        return dev->path();

    // setup correct path for UDisk operations, need sd* instead of /dev/sd*
    devpath.remove(0, devpath.lastIndexOf("/")+1);
    udisk_path.append(devpath);

    ret = get_property(udisk_path, "MountPoints", UDISK_INTERFACE_FILESYSTEM);
    if(!ret.isValid())
        return QString();

    /* try to read mountpoint as string
     * as this is an array of QByteArrays above helper functions extract
     * all mountpoints as a QStringList */
    if(!variantToString(ret, mp))
        return QString();

    if(mp.isEmpty())
        return QString();

    qDebug() << tr("qhimddetection: detected mountpoint for %1 at %2 : %3").arg(dev->name()).arg(dev->deviceFile()).arg(mp.first());
    /* use first detected mountpoint and set the path of the device */
    dev->setPath(mp.at(0));
    return mp.at(0);
}

bool QHiMDLinuxDetection::check_serial(QString udisk_drive_path, QString serial)
{
    QVariant ret;
    QStringList reply;

    ret = get_property(udisk_drive_path, "Serial", UDISK_INTERFACE_DRIVE);
    if(!ret.isValid())
        return false;

    if(!variantToString(ret, reply))
        return false;

    if(reply.at(0).compare(serial, Qt::CaseInsensitive))
        return false;

    return true;
}

QString QHiMDLinuxDetection::deviceFile_from_serial(QString serial)
{
    char drive ='a';
    QString name, u_path;
    bool validDrive = true;

    while(validDrive) {
        if(!(validDrive = udisks_drive_path(drive, u_path)))
                break;

        if(!u_path.isEmpty()) {
            if(drive_is_himd(u_path, name)) {
                if(check_serial(u_path, serial))
                    return QString("/dev/sd").append(drive);
            }
        }
        ++drive;
    }
    return QString();
}

void QHiMDLinuxDetection::scan_for_himd_devices()
{
    char drive ='a';
    QString name, path;
    bool validDrive = true;

    /* skip enumeration if using libusb hotplug events */
    if(ctx)
        return;

    while(validDrive) {
        if(!(validDrive = udisks_drive_path(drive, path))) {
                break;
        }

        if(!path.isEmpty()) {
            if(drive_is_himd(path, name)) {
                qDebug() << tr("qhimddetection: himd device %1 detected at %2").arg(name).arg(QString("/dev/sd").append(drive));
                add_himddevice(QString("/dev/sd").append(drive), name, NULL);
            }
        }
        ++drive;
    }
}

QMDDevice *QHiMDLinuxDetection::find_by_deviceFile(QString file)
{
    QMDDevice * mddev;

    foreach(mddev, dlist)
    {
        if(mddev->deviceFile() == file)
            return mddev;
    }
    return NULL;
}

void QHiMDLinuxDetection::add_himddevice(QString path, QString name, libusb_device * dev)
{
    QString mp;
    QHiMDDevice * new_device;
    struct libusb_device_descriptor desc;
    libusb_device_handle * h = NULL;
    unsigned char serial[13];
    int rc = 0;

    if(path.isEmpty() && !dev)
            return;

    /* return if device already exists in the device list */
    if(dev && find_by_libusbDevice(dev))
        return;

     /* try to find device file from serial if path is not set (using libusb hotplug events) */
    if(path.isEmpty()) {
        libusb_get_device_descriptor(dev, &desc);
        rc = libusb_open(dev, &h);
        if(rc != LIBUSB_SUCCESS)
            return;
        memset(serial, 0, 13);
        libusb_get_string_descriptor_ascii(h, desc.iSerialNumber, serial, 13);
        libusb_close(h);
        path = deviceFile_from_serial(QString((const char *)serial));
            if(path.isEmpty())
                return;
    }

    /* return if device by path already exists in the device list */
    if (find_by_deviceFile(path))
        return;

    new_device = new QHiMDDevice();

    new_device->setDeviceFile(path);
    new_device->setLibusbDevice(dev);
    new_device->setName(name);
    new_device->setBusy(false);
    mp = mountpoint(new_device);
    new_device->setPath(mp);
    new_device->setMdInserted(!mp.isEmpty());

    dlist.append(new_device);
    emit deviceListChanged(dlist);
}

//  MOC_SKIP_END
