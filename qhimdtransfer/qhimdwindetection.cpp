#include <QDebug>
#include <QList>
#include <QWidget>
#include <qhimddetection.h>

#define WINVER 0x0500

#include <windows.h>
#include <dbt.h>
#include <setupapi.h>
#include <initguid.h>       // needed to handle GUIDs
#include <ddk/ntddstor.h>   // needed for handling storage devices
#include <ddk/cfgmgr32.h>   // needed for CM_Get_Child function

struct win_himd_device : himd_device {
                    HANDLE devhandle;
                    HDEVNOTIFY himdChange;
                    };

static const GUID my_GUID_IO_MEDIA_ARRIVAL =
    {0xd07433c0, 0xa98e, 0x11d2, {0x91, 0x7a, 0x00, 0xa0, 0xc9, 0x06, 0x8f, 0xf3} };

static const GUID my_GUID_IO_MEDIA_REMOVAL =
    {0xd07433c1, 0xa98e, 0x11d2, {0x91, 0x7a, 0x00, 0xa0, 0xc9, 0x06, 0x8f, 0xf3} };

static const int my_DBT_CUSTOMEVENT = 0x8006;


static bool is_himddevice(QString devID, QString & name);
static QString get_deviceID_from_driveletter(char i);
static bool identified(QString devpath, QString & name);
static QString FindPath(unsigned long unitmask);

class QHiMDWinDetection : public QHiMDDetection, private QWidget {

public:
    void scan_for_himd_devices();
    QHiMDWinDetection(QObject * parent = NULL);
    ~QHiMDWinDetection();
    win_himd_device *find_by_path(QString path);

private:
    HDEVNOTIFY hDevNotify;
    win_himd_device *find_by_handle(HANDLE devhandle);
    win_himd_device *win_dev_at(int idx);
    void add_himddevice(QString path, QString name);
    void remove_himddevice(QString path);
    void add_himd(HANDLE devhandle);
    void remove_himd(HANDLE devhandle);
    HDEVNOTIFY register_mediaChange(HANDLE devhandle);
    void unregister_mediaChange(HDEVNOTIFY himd_change);
    bool winEvent(MSG * msg, long * result);
};


QHiMDDetection * createDetection(QObject * parent)
{
    return new QHiMDWinDetection(parent);
}

QHiMDWinDetection::QHiMDWinDetection(QObject * parent) 
  : QHiMDDetection(parent), QWidget(0)
{
    // ask for Window ID to have Qt create the window.
    (void)winId();
}

QHiMDWinDetection::~QHiMDWinDetection()
{
    while (!device_list.isEmpty())
        remove_himddevice(device_list.at(0)->path);
}

void QHiMDWinDetection::scan_for_himd_devices()
{
    unsigned long drives = GetLogicalDrives();
    char drive[] = "A:\\";
    QString name, devID;

    for (; drive[0] <= 'Z'; ++drive[0])
    {
        if (drives & 0x1)
        {
            if(GetDriveTypeA(drive) == DRIVE_REMOVABLE)
            {
                devID = get_deviceID_from_driveletter(drive[0]);
                if(!devID.isEmpty() && !devID.contains("Floppy", Qt::CaseInsensitive))
                {
                    if(is_himddevice(devID, name))
                        add_himddevice(QString(drive[0]) + ":/", name);
                }
            }
        }
        drives = drives >> 1;
    }
    return;
}

win_himd_device *QHiMDWinDetection::win_dev_at(int idx)
{
    return static_cast<win_himd_device*>(device_list.at(idx));
}

win_himd_device *QHiMDWinDetection::find_by_path(QString path)
{
    return static_cast<win_himd_device*>(QHiMDDetection::find_by_path(path));
}

win_himd_device *QHiMDWinDetection::find_by_handle(HANDLE devhandle)
{
    for (int i = 0; i < device_list.size(); i++)
        if(win_dev_at(i)->devhandle == devhandle)
            return win_dev_at(i);

    return NULL;
}

static QString get_deviceID_from_driveletter(char i)
{
    char subkey[] = "\\DosDevices\\X:";
    DWORD valuesize;
    HKEY key;
    int res;
    QString devname;

    subkey[12] = i;
    res = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SYSTEM\\MountedDevices", NULL, KEY_QUERY_VALUE, &key);
    if(res != ERROR_SUCCESS)
        return QString();

    if(RegQueryValueExA(key, subkey, NULL, NULL, NULL, &valuesize) == ERROR_SUCCESS)
    {
        char *value = new char[valuesize];
        res = RegQueryValueExA(key, subkey, NULL, NULL,(LPBYTE) value, &valuesize);
        if(res == ERROR_SUCCESS)
        {
            devname = QString::fromUtf16((ushort*)value);
            devname.remove(0,4);                            // modify devname to make a valid device ID
            devname.truncate(devname.indexOf("{") -1);
            devname = devname.toUpper();
            devname.replace("#", "\\");
        }
        delete[] value;
    }
    RegCloseKey(key);
    return devname;
}

static bool is_himddevice(QString devID, QString & name)
{
    DEVINST devinst;
    DEVINST devinstparent;
    unsigned long buflen;
    QString recname, devicepath;

    CM_Locate_DevNodeA(&devinst, devID.toAscii().data(), NULL);
    CM_Get_Parent(&devinstparent, devinst, NULL);

    if(devID.contains("RemovableMedia", Qt::CaseInsensitive))    // on Windows XP: get next parent device instance
        CM_Get_Parent(&devinstparent, devinstparent, NULL);

    CM_Get_Device_ID_Size(&buflen, devinstparent, 0);
    wchar_t *buffer = new wchar_t[buflen];
    CM_Get_Device_ID(devinstparent, buffer, buflen, 0);
    devicepath = QString::fromWCharArray(buffer);
    delete[] buffer;

    if(identified( devicepath, recname))
    {
        name = recname;
        return true;
    }
    return false;
}

static bool identified(QString devpath, QString & name)
{
    int vid = devpath.mid(devpath.indexOf("VID") + 4, 4).toInt(NULL,16);
    int pid = devpath.mid(devpath.indexOf("PID") + 4, 4).toInt(NULL,16);
    const char * devname = identify_usb_device(vid, pid);
    if (devname)
    {
        name = devname;
        return true;
    }
    return false;
}

void QHiMDWinDetection::add_himddevice(QString path, QString name)
{
    if (find_by_path(path))
        return;

    win_himd_device * new_device = new win_himd_device;
    int k;
    char drv[] = "\\\\.\\X:";
    QByteArray device = "\\\\.\\PHYSICALDRIVE";
    char file[] = "X:\\HI-MD.IND";
    DWORD retbytes;
    HANDLE hdev;
    STORAGE_DEVICE_NUMBER sdn;
    OFSTRUCT OFfile;

    drv[4] = path.at(0).toAscii();

    hdev = CreateFileA(drv, NULL , FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                           OPEN_EXISTING, 0, NULL);
    if(hdev == INVALID_HANDLE_VALUE)
        return;

    k = DeviceIoControl(hdev, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(sdn), &retbytes, NULL);
    CloseHandle(hdev);
    if(k != 0)
        device.append(QString::number(sdn.DeviceNumber));

    new_device->devhandle = CreateFileA(device.data(), NULL , FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                           OPEN_EXISTING, 0, NULL);
    if(new_device->devhandle == INVALID_HANDLE_VALUE)
        return;

    new_device->himdChange = register_mediaChange(new_device->devhandle);
    new_device->is_busy = false;
    new_device->path = path;
    new_device->recorder_name = name;

    file[0] = path.at(0).toAscii();
    if(OpenFile(file, &OFfile, OF_EXIST) != HFILE_ERROR)
    {
        new_device->md_inserted = true;
        emit himd_found(new_device->path);
        qDebug() << "himd device at " + new_device->path + " added (" + new_device->recorder_name + ")";
    }
    else
    {
        qDebug() << "himd device at " + new_device->path + " added (" + new_device->recorder_name + ")" + " ; without MD";
        new_device->md_inserted = false;
    }

    device_list.append(new_device);

    return;

}

void QHiMDWinDetection::remove_himddevice(QString path)
{
    win_himd_device * dev = find_by_path(path);
    if (!dev)
        return;

    unregister_mediaChange(dev->himdChange);

    if (dev->devhandle != NULL)
        CloseHandle(dev->devhandle);

    emit himd_removed(dev->path);

    qDebug() << "himd device at " + dev->path + " removed (" + dev->recorder_name + ")";

    device_list.removeAll(dev);
    delete dev;
}

void QHiMDWinDetection::add_himd(HANDLE devhandle)
{
    win_himd_device * dev = find_by_handle(devhandle);
    if (!dev)
        return;

    if(!dev->md_inserted)
    {
        dev->md_inserted = true;
        emit himd_found(dev->path);
        qDebug() << "himd device at " + dev->path + " : md inserted";
    }
    return;
}

void QHiMDWinDetection::remove_himd(HANDLE devhandle)
{
    win_himd_device * dev = find_by_handle(devhandle);
    if (!dev)
        return;

    if(dev->md_inserted)
    {
        dev->md_inserted = false;
        emit himd_removed(dev->path);
        qDebug() << "himd device at " + dev->path + " :  md removed";
    }
    return;
}

HDEVNOTIFY QHiMDWinDetection::register_mediaChange(HANDLE devhandle)
{
    DEV_BROADCAST_HANDLE filter;

    ZeroMemory( &filter, sizeof(filter) );
    filter.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
    filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
    filter.dbch_handle = devhandle;
    filter.dbch_eventguid = my_GUID_IO_MEDIA_ARRIVAL;  // includes GUID_IO_MEDIA_REMOVAL notification

    return RegisterDeviceNotification( this->winId(), &filter, DEVICE_NOTIFY_WINDOW_HANDLE);

}

void QHiMDWinDetection::unregister_mediaChange(HDEVNOTIFY himd_change)
{
    if(himd_change != NULL)
        UnregisterDeviceNotification(himd_change);
}

bool QHiMDWinDetection::winEvent(MSG * msg, long * result)
    {
        QString name, devID, path ;
        if(msg->message == WM_DEVICECHANGE)
        {
            PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR )msg->lParam;
            switch(msg->wParam)
            {
                case DBT_DEVICEARRIVAL :
                {
                    if(pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
                    {
                        PDEV_BROADCAST_VOLUME pHdrv = (PDEV_BROADCAST_VOLUME)pHdr;
                        path = FindPath(pHdrv->dbcv_unitmask);
                        devID = get_deviceID_from_driveletter(path.at(0).toAscii());
                        if(!devID.isEmpty())
                        {
                            if(is_himddevice(devID, name))
                            {
                                qDebug() << "Message:DBT_DEVICEARRIVAL for drive " + path;
                                add_himddevice(path, name);
                            }
                        }
                    }
                    break;
                }
                case DBT_DEVICEREMOVECOMPLETE :
                {
                    if(pHdr->dbch_devicetype == DBT_DEVTYP_VOLUME)
                    {
                        PDEV_BROADCAST_VOLUME pHdrv = (PDEV_BROADCAST_VOLUME)pHdr;
                        path = FindPath(pHdrv->dbcv_unitmask);
                        qDebug() << "Message:DBT_DEVICEREMOVECOMPLETE for drive " + path;
                        remove_himddevice(path);
                    }
                    break;
                }
                case DBT_DEVICEQUERYREMOVE :
                {
                    if(pHdr->dbch_devicetype & DBT_DEVTYP_HANDLE)
                    {
                        PDEV_BROADCAST_HANDLE pHdrh = (PDEV_BROADCAST_HANDLE)pHdr;
                        win_himd_device *dev = find_by_handle(pHdrh->dbch_handle);
                        if(!dev)
                        {
                            qDebug() << "Message:DBT_DEVICEQUERYREMOVE for unknown device " << pHdrh->dbch_handle;
                            break;
                        }
                        if(dev->is_busy)
                        {
                            *result = BROADCAST_QUERY_DENY;
                            qDebug() << "Message:DBT_DEVICEQUERYREMOVE for drive " + path + " denied: transfer in progress";
                            return true;
                        }
                        else
                        {
                            qDebug() << "Message:DBT_DEVICEQUERYREMOVE requested";
                            remove_himddevice(dev->path);
                        }
                    }
                    break;
                }
                case my_DBT_CUSTOMEVENT :
                {
                    if(pHdr->dbch_devicetype & DBT_DEVTYP_HANDLE)
                    {
                        PDEV_BROADCAST_HANDLE pHdrh = (PDEV_BROADCAST_HANDLE)pHdr;
                        if (pHdrh->dbch_eventguid == my_GUID_IO_MEDIA_ARRIVAL)
                        {
                            qDebug() << "Message:DBT_CUSTOMEVENT - GUID_IO_MEDIA_ARRIVAL";
                            add_himd(pHdrh->dbch_handle);
                            break;
                        }
                        if (pHdrh->dbch_eventguid == my_GUID_IO_MEDIA_REMOVAL)
                        {
                            qDebug() << "Message:DBT_CUSTOMEVENT - GUID_IO_MEDIA_REMOVAL";
                            remove_himd(pHdrh->dbch_handle);
                            break;
                        }
                    }
                    break;
                }
                default: return false;  // skip unknown/unused messages
            }
            *result = TRUE;
            return true;
        }
        return false;
    }

static QString FindPath (unsigned long unitmask)
{
   char i;

   for (i = 0; i < 26; ++i)
   {
      if (unitmask & 0x1)
         break;
      unitmask = unitmask >> 1;
   }
   return QString(i + 'A') + ":/";
}
