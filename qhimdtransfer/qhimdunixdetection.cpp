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


/* qhimdtransfer adaptor class, can be accessed through dbus session bus with
 * service=org.linux-minidisc.qhimdtransfer
 * path=/QHiMDUnixDetection
 * interface=org.linux-minidisc.qhimdtransfer.QHiMDUnixDetection
 *
 * following methods provided
 * "AddMDDevice" and "RemoveMDDevice"
 * with args: QString deviceFile, int vid, int pid
 */
class QHiMDAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.linux-minidisc.qhimdtransfer")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.linux-minidisc.qhimdtransfer\">\n"
"    <method name=\"AddMDDevice\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"deviceFile\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"vendorId\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"productId\"/>\n"
"    </method>\n"
"    <method name=\"RemoveMDDevice\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"deviceFile\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"vendorId\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"productId\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")
public:
    QHiMDAdaptor(QObject *parent) : QDBusAbstractAdaptor(parent) {}
    virtual ~QHiMDAdaptor() {}
};


class QHiMDUnixDetection : public QHiMDDetection{
    Q_OBJECT

    QHiMDAdaptor *had;
    QDBusConnection dbus_sys;  // system bus connection: needed for getting properties for a specific device
    QDBusConnection dbus_ses;  // session bus connection: needed for providing method calls AddMDDevice and RemoveMDDevice

public:
    QHiMDUnixDetection(QObject * parent = NULL);
    ~QHiMDUnixDetection() {}
    virtual QString mountpoint(QMDDevice *dev);
    virtual void scan_for_himd_devices();

private:
    QVariant get_property(QString udiskPath, QString property, QString interface);
    bool drive_is_himd(QString udisk_drive_path, QString &name);
    bool udisks_drive_path(char drive, QString &drive_path);
    QMDDevice *find_by_deviceFile(QString file);
    void add_himddevice(QString file, QString name);
    virtual void remove_himddevice(QString file);

public slots:
    void AddMDDevice(QString deviceFile, int vid, int pid);
    void RemoveMDDevice(QString deviceFile, int vid, int pid);
};

/* force qmake to run moc on this .cpp file */
#include "qhimdunixdetection.moc"
//  MOC_SKIP_BEGIN

QHiMDDetection * createDetection(QObject * parent)
{
    return new QHiMDUnixDetection(parent);
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

QHiMDUnixDetection::QHiMDUnixDetection(QObject *parent)
  : QHiMDDetection(parent), had(new QHiMDAdaptor(this)),
    dbus_sys(QDBusConnection::connectToBus( QDBusConnection::SystemBus, "system" ) ),
    dbus_ses(QDBusConnection::connectToBus(QDBusConnection::SessionBus, "org.linux-minidisc.qhimdtransfer"))
{
    if(!dbus_sys.isConnected())
        qDebug() << tr("cannot connect to system bus");
    if(!dbus_ses.isConnected())
        qDebug() << tr("cannot connect to session bus");

    if(!dbus_ses.registerObject("/QHiMDUnixDetection", this, QDBusConnection::ExportAllSlots))
        qDebug() << tr("cannot register dbus interface object ");

    // register interface to session bus to make it visible to all other connections
    dbus_ses.interface()->registerService("org.linux-minidisc.qhimdtransfer");
    dbus_ses.interface()->startService("org.linux-minidisc.qhimdtransfer");

    // now connect method calls to our slots
    QDBusConnection::sessionBus().connect("org.linux-minidisc.qhimdtransfer", "/org/linux-minidisc/qhimdtransfer/QHiMDUnixDetection", "org.linux-minidisc.qhimdtransfer", "AddMDDevice", this, SLOT(AddMDDevice(QString, int, int)));
    QDBusConnection::sessionBus().connect("org.linux-minidisc.qhimdtransfer", "/org/linux-minidisc/qhimdtransfer/QHiMDUnixDetection", "org.linux-minidisc.qhimdtransfer", "RemoveMDDevice", this, SLOT(RemoveMDDevice(QString,int,int)));
}

QVariant QHiMDUnixDetection::get_property(QString udiskPath, QString property, QString interface)
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

bool QHiMDUnixDetection::drive_is_himd(QString udisk_drive_path, QString &name)
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

bool QHiMDUnixDetection::udisks_drive_path(char drive, QString &drive_path)
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

QString QHiMDUnixDetection::mountpoint(QMDDevice *dev)
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

    qDebug() << tr("qhimdtransfer detection: detected mountpoint for %1 at %2 : %3").arg(dev->name()).arg(dev->deviceFile()).arg(mp.first());
    /* use first detected mountpoint and set the path of the device */
    dev->setPath(mp.at(0));
    return mp.at(0);
}

void QHiMDUnixDetection::scan_for_himd_devices()
{
    char drive ='a';
    QString name, path;
    bool validDrive = true;

    while(validDrive) {
        /* end enumeration if drive does not exixt */
        if(!(validDrive = udisks_drive_path(drive, path))) {
                break;
        }
        /* continue with next drive if it exists but is not a valid udisks drive */
        if(!path.isEmpty()) {
            /* check if this is a himd device */
            if(drive_is_himd(path, name)) {
                qDebug() << tr("qhimdtransfer detection: himd device %1 detected at %2").arg(name).arg(QString("/dev/sd").append(drive));
                add_himddevice(QString("/dev/sd").append(drive), name);
            }
        }
        ++drive;
    }
}

QMDDevice *QHiMDUnixDetection::find_by_deviceFile(QString file)
{
    QMDDevice * mddev;

    foreach(mddev, dlist)
    {
        if(mddev->deviceFile() == file)
            return mddev;
    }
    return NULL;
}

void QHiMDUnixDetection::add_himddevice(QString file, QString name)
{
    QString path;
    if (find_by_deviceFile(file))
        return;

    QHiMDDevice * new_device = new QHiMDDevice();

    new_device->setDeviceFile(file);
    new_device->setName(name);
    new_device->setBusy(false);
    path = mountpoint(new_device);
    new_device->setPath(path);
    new_device->setMdInserted(!path.isEmpty());

    dlist.append(new_device);
    emit deviceListChanged(dlist);
}

void QHiMDUnixDetection::remove_himddevice(QString file)
{
    int index = -1;
    QMDDevice * dev;

    if (!(dev = find_by_deviceFile(file)))
        return;

    index = dlist.indexOf(dev);

    if(dev->isOpen())
        dev->close();

    delete dev;
    dev = NULL;

    dlist.removeAt(index);

    emit deviceListChanged(dlist);
}

void QHiMDUnixDetection::AddMDDevice(QString deviceFile, int vid, int pid)
{
    QString name = QString(identify_usb_device(vid, pid));

    // check if this is valid minidisc device depending on vendor and product id, for all known devices identify_usb_device() should return a name
    if(name.isEmpty())
        return;

    /* check if it is a netmd device, reenumerate netmd devices at this point to make libnetmd find it
     */
    if(name.contains("NetMD"))
    {
        qDebug() << tr("qhimdtransfer detection: netmd device detected: %1").arg(name);
        rescan_netmd_devices();
        return;
    }

    // check if driver file is /dev/sd*, this is what we need, for future use (formating etc.) also check for /dev/sg* scsi driver file
    if(!deviceFile.startsWith("/dev/sd"))
        return;

    qDebug() << tr("qhimdtransfer detection: himd device detected at %1: %2").arg(deviceFile).arg(name);

    add_himddevice(deviceFile,name);

}

void QHiMDUnixDetection::RemoveMDDevice(QString deviceFile, int vid, int pid)
{
    QString name = QString(identify_usb_device(vid, pid));

    // check if this is valid minidisc device, for all known devices identify_usb_device() should return a name
    if(name.isEmpty())
        return;

    if(name.contains("NetMD"))
    {
        qDebug() << tr("qhimdtransfer detection: netmd device removed: %1").arg(name);
        rescan_netmd_devices();
        return;
    }

    // check if driver file is /dev/sd*, this is what we need, for future use (formating etc.) also check for /dev/sg* scsi driver file
    if(!deviceFile.startsWith("/dev/sd"))
        return;

    qDebug() << tr("qhimdtransfer detection: himd device removed at %1: %2").arg(deviceFile).arg(name);

    remove_himddevice(deviceFile);
}

//  MOC_SKIP_END
