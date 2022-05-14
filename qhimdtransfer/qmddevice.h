#ifndef QMDDEVICE_H
#define QMDDEVICE_H

#include <QString>
#include <QStringList>

#include <qmdtrack.h>
#include "qhimduploaddialog.h"

#if defined(_WIN32)
#include <windows.h>
#endif /* _WIN32 */

enum device_type {
    NO_DEVICE,
    NETMD_DEVICE,
    HIMD_DEVICE
};

class QMDDevice : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(QMDDevice)

    QString recorder_name;
    bool is_busy;
protected:
    QString device_path;
    QString device_file;
    enum device_type dev_type;
    bool is_open;
    unsigned int trk_count;
    bool md_inserted;
    void * mdChange;
    libusb_device * ldev;
public:
    explicit QMDDevice();
    virtual ~QMDDevice();
    virtual enum device_type deviceType();
    virtual void setPath(QString path);
    virtual QString path();
    virtual void setName(QString name);
    virtual QString name();
    virtual void setDeviceFile(QString devfile);
    virtual QString deviceFile();
    virtual void setBusy(bool busy);
    virtual bool isBusy();
    virtual QString open() {return QString();}
    virtual void close() {}
    virtual bool isOpen() {return is_open;}
    virtual QString discTitle() {return QString();}
    virtual void setMdInserted(bool inserted);
    virtual bool mdInserted();
    virtual void registerMdChange(void * regMdChange);
    virtual void * MdChange();
    virtual void setLibusbDevice(libusb_device * dev);
    virtual libusb_device * libusbDevice();
    virtual int trackCount() {return trk_count;}
    virtual QStringList downloadableFileExtensions() const = 0;
    virtual void checkfile(QString UploadDirectory, QString &filename, QString extension);
    virtual void upload(TransferTask &task, QString path) = 0;
    virtual void download(TransferTask &task) = 0;
    virtual bool canUpload() = 0;
    virtual bool isWritable() = 0;
    virtual bool canFormatDisk() = 0;
    virtual bool formatDisk() = 0;
    virtual QString getRecordedLabelText() = 0;
    virtual QString getAvailableLabelText() = 0;

    void batchUpload(QHiMDUploadDialog *dialog, QMDTrackIndexList tlist, QString path);
    void batchDownload(QHiMDUploadDialog *dialog, const QStringList &filenames);

    virtual qlonglong getTrackBlockCount(int trackIndex) = 0;

    virtual bool hasPlaybackControls() const { return false; }

    virtual void play() {}
    virtual void pause() {}
    virtual void stop() {}

    virtual void startRewind() {}
    virtual void startFastForward() {}

    virtual void gotoPreviousTrack() {}
    virtual void gotoNextTrack() {}

#if defined(_WIN32)
    HANDLE winDeviceHandle;
#endif /* _WIN32 */

signals:
    void opened();
    void closed();
};

class QNetMDDevice : public QMDDevice {

    netmd_device * netmd;
    netmd_dev_handle * devh;

    int upload_reported_track_blocks;
    int upload_total_track_blocks;

    int download_reported_file_blocks;
    int download_total_file_blocks;

    QString recordedLabelText;
    QString availableLabelText;
public:
    explicit QNetMDDevice();
    virtual ~QNetMDDevice();
    virtual void setUsbDevice(netmd_device * dev);
    virtual netmd_device * usbDevice() {return netmd;}
    virtual QString open();
    virtual void close();
    virtual QString discTitle();
    virtual QNetMDTrack netmdTrack(unsigned int trkindex);
    virtual void upload(TransferTask &task, QString path);
    virtual void download(TransferTask &task);
    virtual bool canUpload();
    virtual QStringList downloadableFileExtensions() const;

    netmd_dev_handle *deviceHandle() { return devh; }

    virtual bool isWritable();
    virtual bool canFormatDisk();
    virtual bool formatDisk();

    virtual QString getRecordedLabelText();
    virtual QString getAvailableLabelText();

    virtual qlonglong getTrackBlockCount(int trackIndex);

    virtual bool hasPlaybackControls() const { return true; }

    virtual void play();
    virtual void pause();
    virtual void stop();

    virtual void startRewind();
    virtual void startFastForward();

    virtual void gotoPreviousTrack();
    virtual void gotoNextTrack();
};

class QHiMDDevice : public QMDDevice {

    struct himd * himd;
    QTime total_duration;

private:
    QString dumpmp3(TransferTask &task, const QHiMDTrack &trk, QString file);
    QString dumpoma(TransferTask &task, const QHiMDTrack & track, QString file);
    QString dumppcm(TransferTask &task, const QHiMDTrack &track, QString file);
public:
    explicit QHiMDDevice();
    virtual ~QHiMDDevice();
    virtual QString open();
    virtual void close();
    virtual QString discTitle();
    virtual QHiMDTrack himdTrack(unsigned int trkindex);
    virtual void upload(TransferTask &task, QString path);
    virtual void download(TransferTask &task);
    virtual bool canUpload();
    virtual QStringList downloadableFileExtensions() const;

    struct himd *deviceHandle() { return himd; }

    virtual bool isWritable();
    virtual bool canFormatDisk();
    virtual bool formatDisk();

    virtual QString getRecordedLabelText();
    virtual QString getAvailableLabelText();

    virtual qlonglong getTrackBlockCount(int trackIndex);
};

#endif // QMDDEVICE_H
