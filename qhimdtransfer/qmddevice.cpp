#include <qmddevice.h>
#include <QMessageBox>
#include <QApplication>
#include <QFile>
#include <QProgressDialog>
#include <QDir>
#include "wavefilewriter.h"
#include "himd.h"

#include <tlist.h>
#include <fileref.h>
#include <tfile.h>
#include <tag.h>
#include <libusb.h>

/* common device members */
QMDDevice::QMDDevice() : dev_type(NO_DEVICE)
{
}

QMDDevice::~QMDDevice()
{
    close();
    ldev = NULL;
}

enum device_type QMDDevice::deviceType()
{
    return dev_type;
}

void QMDDevice::setPath(QString path)
{
    device_path = path;
    md_inserted = !path.isEmpty();
}

QString QMDDevice::path()
{
    return device_path;
}

void QMDDevice::setName(QString name)
{
    recorder_name = name;
}

QString QMDDevice::name()
{
    return recorder_name;
}

void QMDDevice::setDeviceFile(QString devfile)
{
    device_file = devfile;
}

QString QMDDevice::deviceFile()
{
    return device_file;
}

void QMDDevice::setBusy(bool busy)
{
    is_busy = busy;
}

bool QMDDevice::isBusy()
{
    return is_busy;
}

void QMDDevice::setMdInserted(bool inserted)
{
    md_inserted = inserted;
}

bool QMDDevice::mdInserted()
{
    return md_inserted;
}

void QMDDevice::setDeviceHandle(void * devicehandle)
{
    devhandle = devicehandle;
}

void * QMDDevice::deviceHandle()
{
    return devhandle;
}

void QMDDevice::registerMdChange(void * regMdChange)
{
    mdChange = regMdChange;
}

void * QMDDevice::MdChange()
{
    return mdChange;
}

void QMDDevice::setLibusbDevice(libusb_device * dev)
{
    ldev = dev;
}

libusb_device *QMDDevice::libusbDevice()
{
    return ldev;
}


QStringList QMDDevice::downloadableFileExtensions() const
{
    if(dev_type == NETMD_DEVICE)
        return QStringList() << "wav";

    if(dev_type == HIMD_DEVICE)
        return QStringList() << "mp3";

    return QStringList();
}

void QMDDevice::checkfile(QString UploadDirectory, QString &filename, QString extension)
{
    QFile f;
    QString newname;
    int i = 2;

    f.setFileName(UploadDirectory + "/" + filename + extension);
    while(f.exists())
    {
        newname = filename + " (" + QString::number(i) + ")";
        f.setFileName(UploadDirectory + "/" + newname + extension);
        i++;
    }
    if(!newname.isEmpty())
        filename = newname;
}


/* netmd device members */
QNetMDDevice::QNetMDDevice()
{
    dev_type = NETMD_DEVICE;
    devh = NULL;
    netmd = NULL;
    is_open = false;

    upload_reported_track_blocks = 0;
    upload_total_track_blocks = 0;
}

QNetMDDevice::~QNetMDDevice()
{
    close();
}

void QNetMDDevice::setUsbDevice(netmd_device * dev)
{
    netmd = dev;
}

QString QNetMDDevice::open()
{
    uint8_t i = 0;
    netmd_error error;
    char buffer[256];

    if(!netmd)
        return tr("netmd_device not set, use setUsbDevice() function first");

    if((error = netmd_open(netmd, &devh)) != NETMD_NO_ERROR)
        return tr("Error opening netmd: %1").arg(netmd_strerror(error));

    current_md = netmd_minidisc_load(devh);

    /* generate track count first, needed by QNetMDTracksModel */
    while(netmd_request_title(devh, i, buffer, sizeof(buffer)) >= 0)
        i++;

    trk_count = i;

    is_open = true;
    md_inserted = true;
    emit opened();
    return QString();
}

void QNetMDDevice::close()
{
    if(!devh)
        return;

    netmd_minidisc_free(current_md);
    current_md = NULL;

    netmd_close(devh);
    devh = NULL;

    is_open = false;
    trk_count = 0;
    md_inserted = false;
    emit closed();
}

QString QNetMDDevice::discTitle()
{
    return QString(netmd_minidisc_get_disc_name(current_md));
}

QNetMDTrack QNetMDDevice::netmdTrack(unsigned int trkindex)
{
    return QNetMDTrack(devh, current_md, trkindex);
}

bool QNetMDDevice::canUpload()
{
    return netmd_dev_can_upload(devh);
}


static void
on_recv_progress(struct netmd_recv_progress *recv_progress)
{
    QNetMDDevice *self = static_cast<QNetMDDevice *>(recv_progress->user_data);

    self->onUploadProgress(recv_progress->progress);
}

void QNetMDDevice::onUploadProgress(float progress)
{
    // This is weird due to the QHiMDUploadDialog API working only with "blocks"
    int progress_blocks = progress * upload_total_track_blocks;
    while (upload_reported_track_blocks < progress_blocks) {
        uploadDialog.blockTransferred();
        ++upload_reported_track_blocks;
    }

    QApplication::processEvents();
}

void QNetMDDevice::upload(unsigned int trackidx, QString path)
{
    QString oldcwd = QDir::currentPath();

    QDir::setCurrent(path);

    QNetMDTrack track = netmdTrack(trackidx);
    uploadDialog.starttrack(track, track.title());

    upload_reported_track_blocks = 0;
    upload_total_track_blocks = track.blockcount();

    struct netmd_track_info info;
    netmd_error err = netmd_get_track_info(devh, trackidx, &info);

    if (err == NETMD_NO_ERROR) {
        char *filename = netmd_recv_get_default_filename(&info);

        err = netmd_recv_track(devh, trackidx, filename, on_recv_progress, this);
        onUploadProgress(1.f);

        netmd_free_string(filename);
    }

    if (err == NETMD_NO_ERROR) {
        uploadDialog.trackSucceeded();
    } else {
        uploadDialog.trackFailed(netmd_strerror(err));
    }

    QDir::setCurrent(oldcwd);
}


void QNetMDDevice::batchUpload(QMDTrackIndexList tlist, QString path)
{
    int allblocks = 0;

    setBusy(true);

    for(int i = 0;i < tlist.length(); i++) {
        allblocks += netmdTrack(tlist.at(i)).blockcount();
    }

    uploadDialog.init(tlist.length(), allblocks);

    for(int i = 0; i < tlist.length(); i++) {
        upload(tlist[i], path);
        QApplication::processEvents();
        if(uploadDialog.upload_canceled())
            break;
    }

    uploadDialog.finished();
    setBusy(false);
}

static void
on_send_progress(struct netmd_send_progress *send_progress)
{
    QProgressDialog *dialog = static_cast<QProgressDialog *>(send_progress->user_data);

    dialog->setValue(100 * send_progress->progress);
    dialog->setLabelText(send_progress->message);

    QApplication::processEvents();
}

bool QNetMDDevice::download(const QString &filename)
{
    QProgressDialog progress("Please wait...", QString(), 0, 100);
    progress.setWindowModality(Qt::WindowModal);

    int result = netmd_send_track(devh, filename.toUtf8().data(), NULL, on_send_progress, &progress);

    return result == NETMD_NO_ERROR;
}

/* himd device members */

QHiMDDevice::QHiMDDevice()
{
    dev_type = HIMD_DEVICE;
    himd = NULL;
    is_open = false;
}

QHiMDDevice::~QHiMDDevice()
{
    close();
}

QString QHiMDDevice::open()
{
    struct himderrinfo status;

    if(!mdInserted())
        return tr("cannot open device, no disc");

    if(himd)  // first close himd if opened
    {
        himd_close(himd);
        delete himd;
        himd = NULL;
    }

    himd = new struct himd;
    if(himd_open(himd, device_path.toUtf8(), &status) < 0)
    {
        delete himd;
        himd = NULL;
        return QString::fromUtf8(status.statusmsg);
    }

    trk_count = himd_track_count(himd);
    is_open = true;
    md_inserted = true;
    emit opened();
    return QString();
}

void QHiMDDevice::close()
{
    if(!himd)
        return;

    himd_close(himd);
    delete himd;
    himd = NULL;

    is_open = false;
    trk_count = 0;
    emit closed();
}

QHiMDTrack QHiMDDevice::himdTrack(unsigned int trkindex)
{
    return QHiMDTrack(himd, trkindex);
}

QString QHiMDDevice::dumpmp3(const QHiMDTrack &trk, QString file)
{
    QString errmsg;
    struct himd_mp3stream str;
    struct himderrinfo status;
    unsigned int len;
    const unsigned char * data;
    QFile f(file);

    if(!f.open(QIODevice::ReadWrite))
    {
        return tr("Error opening file for MP3 output");
    }
    if(!(errmsg = trk.openMpegStream(&str)).isNull())
    {
        f.remove();
        return tr("Error opening track: ") + errmsg;
    }
    while(himd_mp3stream_read_block(&str, &data, &len, NULL, &status) >= 0)
    {
        if(f.write((const char*)data,len) == -1)
        {
            errmsg = tr("Error writing audio data");
            goto clean;
        }
        uploadDialog.blockTransferred();
        QApplication::processEvents();
        if(uploadDialog.upload_canceled())
        {
            errmsg = tr("upload aborted by the user");
            goto clean;
        }

    }
    if(status.status != HIMD_STATUS_AUDIO_EOF)
        errmsg = tr("Error reading audio data: ") + status.statusmsg;

clean:
    f.close();
    himd_mp3stream_close(&str);
    if(!errmsg.isNull())
        f.remove();
    return errmsg;
}

static inline TagLib::String QStringToTagString(const QString & s)
{
    return TagLib::String(s.toUtf8().data(), TagLib::String::UTF8);
}

static void addid3tag(QString title, QString artist, QString album, QString file)
{
#ifdef Q_OS_WIN
    TagLib::FileRef f(file.toStdWString().c_str());
#else
    TagLib::FileRef f(file.toUtf8().data());
#endif
    TagLib::Tag *t = f.tag();
    t->setTitle(QStringToTagString(title));
    t->setArtist(QStringToTagString(artist));
    t->setAlbum(QStringToTagString(album));
    t->setComment("*** imported from HiMD via QHiMDTransfer ***");
    f.file()->save();
}

QString QHiMDDevice::dumpoma(const QHiMDTrack &track, QString file)
{
    QString errmsg;
    struct himd_nonmp3stream str;
    struct himderrinfo status;
    unsigned int len;
    const unsigned char * data;
    QFile f(file);

    if(!f.open(QIODevice::ReadWrite))
        return tr("Error opening file for ATRAC output");

    if(!(errmsg = track.openNonMpegStream(&str)).isNull())
    {
        f.remove();
        return tr("Error opening track: ") + status.statusmsg;
    }

    if(f.write(track.makeEA3Header()) == -1)
    {
        errmsg = tr("Error writing header");
        goto clean;
    }
    while(himd_nonmp3stream_read_block(&str, &data, &len, NULL, &status) >= 0)
    {
        if(f.write((const char*)data,len) == -1)
        {
            errmsg = tr("Error writing audio data");
            goto clean;
        }
        uploadDialog.blockTransferred();
        QApplication::processEvents();
        if(uploadDialog.upload_canceled())
        {
            errmsg = QString("upload aborted by the user");
            goto clean;
        }
    }
    if(status.status != HIMD_STATUS_AUDIO_EOF)
        errmsg = QString("Error reading audio data: ") + status.statusmsg;

clean:
    f.close();
    himd_nonmp3stream_close(&str);

    if(!errmsg.isNull())
        f.remove();
    return errmsg;
}

QString QHiMDDevice::dumppcm(const QHiMDTrack &track, QString file)
{
    struct himd_nonmp3stream str;
    struct himderrinfo status;
    unsigned int len;
    QString errmsg;
    const unsigned char * data;
    WaveFileWriter waveFile;

    if(!(errmsg = track.openNonMpegStream(&str)).isNull())
    {
        return tr("Error opening track: ") + status.statusmsg;
    }

    if (!waveFile.open(file, 44100, 16, 2)) {
        return tr("Error opening file for WAV output");
    }

    while (himd_nonmp3stream_read_block(&str, &data, &len, NULL, &status) >= 0) {
        if (!waveFile.write_signed_big_endian(reinterpret_cast<const int16_t *>(data), len / 2)) {
            errmsg = tr("Error writing audio data");
            goto clean;
        }

        uploadDialog.blockTransferred();
        QApplication::processEvents();
        if (uploadDialog.upload_canceled()) {
            errmsg = QString("upload aborted by the user");
            goto clean;
        }
    }

    if (status.status != HIMD_STATUS_AUDIO_EOF) {
        errmsg = QString("Error reading audio data: ") + status.statusmsg;
    }

clean:
    waveFile.close();
    himd_nonmp3stream_close(&str);

    if (!errmsg.isNull()) {
        QFile(file).remove();
    }

    return errmsg;
}

void QHiMDDevice::upload(unsigned int trackidx, QString path)
{
    QString filename, errmsg;
    QHiMDTrack track = himdTrack(trackidx);
    QString title = track.title();
    QString artist = track.artist();

    if(title.isEmpty()) {
        filename = tr("Track %1").arg(track.tracknum()+1);
    } else if (artist.isEmpty()) {
        filename = title;
    } else {
        filename = artist + " - " + title;
    }

    uploadDialog.starttrack(track, filename);
    if (!track.copyprotected())
    {
        QString codec = track.codecname();
        if (codec == "MPEG")
        {
            checkfile(path, filename, ".mp3");
            errmsg = dumpmp3 (track, path + "/" + filename + ".mp3");
            if(errmsg.isNull())
                addid3tag (track.title(),track.artist(),track.album(), path + "/" +filename + ".mp3");
        }
        else if (codec == "LPCM")
        {
            checkfile(path, filename, ".wav");
            errmsg = dumppcm (track, path + "/" + filename + ".wav");
        }
        else if (codec == "AT3+" || codec == "AT3 ")
        {
            checkfile(path, filename, ".oma");
            errmsg = dumpoma (track, path + "/" + filename + ".oma");
        }
    }
    else
        errmsg = tr("upload disabled because of DRM encryption");

    if(errmsg.isNull())
        uploadDialog.trackSucceeded();
    else
        uploadDialog.trackFailed(errmsg);

}

void QHiMDDevice::batchUpload(QMDTrackIndexList tlist, QString path)
{
    int allblocks = 0;

    setBusy(true);

    for(int i = 0;i < tlist.length(); i++)
        allblocks += himdTrack(tlist.at(i)).blockcount();

    uploadDialog.init(tlist.length(), allblocks);

    for(int i = 0; i < tlist.length(); i++) {
        upload(tlist[i], path);
        QApplication::processEvents();
        if(uploadDialog.upload_canceled())
            break;
    }

    uploadDialog.finished();
    setBusy(false);
}

bool QHiMDDevice::download(const QString &filename)
{
    return (himd_writemp3(himd, filename.toUtf8().data()) == 0);
}
