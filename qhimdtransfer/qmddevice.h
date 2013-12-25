#ifndef QMDDEVICE_H
#define QMDDEVICE_H

#include <QString>
#include <QStringList>

#include <qmdtrack.h>
#include "qhimduploaddialog.h"

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
    enum device_type dev_type;
    bool is_open;
    unsigned int trk_count;
    bool md_inserted;
    void * devhandle;
    void * mdChange;
    QHiMDUploadDialog uploadDialog;
public:
    explicit QMDDevice();
    virtual ~QMDDevice();
    virtual enum device_type deviceType();
    virtual void setPath(QString path);
    virtual QString path();
    virtual void setName(QString name);
    virtual QString name();
    virtual void setBusy(bool busy);
    virtual bool isBusy();
    virtual QString open() {return QString();}
    virtual void close() {}
    virtual bool isOpen() {return is_open;}
    virtual QString discTitle() {return QString();}
    virtual void setMdInserted(bool inserted);
    virtual bool mdInserted();
    virtual void setDeviceHandle(void * devicehandle);
    virtual void * deviceHandle();
    virtual void registerMdChange(void * regMdChange);
    virtual void * MdChange();
    virtual QMDTrack track(unsigned int trkindex) {return QMDTrack();}
    virtual int trackCount() {return trk_count;}
    virtual QStringList downloadableFileExtensions() const;
    virtual void checkfile(QString UploadDirectory, QString &filename, QString extension);
    virtual void batchUpload(QMDTrackIndexList tlist, QString path) {}
    virtual void upload(unsigned int trackidx, QString path) {}

signals:
    void opened();
    void closed();
};

class QNetMDDevice : public QMDDevice {

    netmd_device * netmd;
    netmd_dev_handle * devh;
    minidisc current_md;
private:
    QString upload_track_blocks(uint32_t length, FILE *file, size_t chunksize);
public:
    explicit QNetMDDevice();
    virtual ~QNetMDDevice();
    virtual void setUsbDevice(netmd_device * dev);
    virtual QString open();
    virtual void close();
    virtual QString discTitle();
    virtual QNetMDTrack netmdTrack(unsigned int trkindex);
    virtual void batchUpload(QMDTrackIndexList tlist, QString path);
    virtual void upload(unsigned int trackidx, QString path);

};

class QHiMDDevice : public QMDDevice {

    struct himd * himd;
private:
    QString dumpmp3(const QHiMDTrack &trk, QString file);
    QString dumpoma(const QHiMDTrack & track, QString file);
    QString dumppcm(const QHiMDTrack &track, QString file);
public:
    explicit QHiMDDevice();
    virtual ~QHiMDDevice();
    virtual QString open();
    virtual void close();
    virtual QHiMDTrack himdTrack(unsigned int trkindex);
    virtual void upload(unsigned int trackidx, QString path);
    virtual void batchUpload(QMDTrackIndexList tlist, QString path);

};

#endif // QMDDEVICE_H
