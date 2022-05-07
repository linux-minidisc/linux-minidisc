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
QMDDevice::QMDDevice()
    : dev_type(NO_DEVICE)
    , uploadDialog(nullptr, QHiMDUploadDialog::UPLOAD)
    , downloadDialog(nullptr, QHiMDUploadDialog::DOWNLOAD)
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

void QMDDevice::batchDownload(const QStringList &filenames)
{
    int allblocks = 0;

    setBusy(true);

    for (const QString &filename: filenames) {
        allblocks += QFileInfo(filename).size();
    }

    downloadDialog.init(filenames.length(), allblocks);

    for (const QString &filename: filenames) {
        download(filename);

        QApplication::processEvents();
        if (downloadDialog.was_canceled()) {
            break;
        }
    }

    downloadDialog.finished();
    setBusy(false);
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

    download_reported_file_blocks = 0;
    download_total_file_blocks = 0;
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
    netmd_error error;

    if(!netmd)
        return tr("netmd_device not set, use setUsbDevice() function first");

    if((error = netmd_open(netmd, &devh)) != NETMD_NO_ERROR)
        return tr("Error opening netmd: %1").arg(netmd_strerror(error));

    trk_count = netmd_get_track_count(devh);

    {
        // TODO: This block has some overlap with netmdcli. We either want to
        // make a "nice" UI with progress bars and stuff for usage, or we want
        // to maybe move some of these as convenience helpers into libnetmd.

        netmd_disc_capacity capacity;
        netmd_get_disc_capacity(devh, &capacity);

        char *recorded = netmd_time_to_string(&capacity.recorded);
        char *total = netmd_time_to_string(&capacity.total);
        char *available = netmd_time_to_string(&capacity.available);

        int total_capacity = netmd_time_to_seconds(&capacity.total);
        int available_capacity = netmd_time_to_seconds(&capacity.available);

        // Need to calculate it like this, because only "total" and "available" are
        // calculated in SP time ("recorded" might be more than that due to Mono/LP2/LP4).
        int percentage_used = (total_capacity != 0) ? (100 * (total_capacity - available_capacity) / total_capacity) : 0;

        netmd_time_double(&capacity.available);
        char *available_lp2_mono = netmd_time_to_string(&capacity.available);

        netmd_time_double(&capacity.available);
        char *available_lp4 = netmd_time_to_string(&capacity.available);

        int ntracks = netmd_get_track_count(devh);

        recordedLabelText = QString("%1 in %2 tracks (%3% of %4 used, %5)")
            .arg(recorded)
            .arg(ntracks)
            .arg(percentage_used)
            .arg(total)
            .arg(netmd_is_disc_writable(devh) ? "writable" : "write-protected");

        availableLabelText = QString("%1 (SP) / %2 (LP2/Mono) / %3 (LP4)")
            .arg(available)
            .arg(available_lp2_mono)
            .arg(available_lp4);

        netmd_free_string(recorded);
        netmd_free_string(total);
        netmd_free_string(available);
        netmd_free_string(available_lp2_mono);
        netmd_free_string(available_lp4);
    }

    is_open = true;
    md_inserted = true;
    emit opened();
    return QString();
}

void QNetMDDevice::close()
{
    if(!devh)
        return;

    netmd_close(devh);
    devh = NULL;

    is_open = false;
    trk_count = 0;
    md_inserted = false;
    emit closed();
}

QString QNetMDDevice::discTitle()
{
    return QString(netmd_get_disc_title(devh));
}

QNetMDTrack QNetMDDevice::netmdTrack(unsigned int trkindex)
{
    return QNetMDTrack(devh, trkindex);
}

bool QNetMDDevice::canUpload()
{
    return netmd_dev_can_upload(devh);
}

QStringList QNetMDDevice::downloadableFileExtensions() const
{
    return QStringList() << "wav";
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

    uploadDialog.blockTransferred(progress_blocks - upload_reported_track_blocks);
    upload_reported_track_blocks = progress_blocks;

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
        if(uploadDialog.was_canceled())
            break;
    }

    uploadDialog.finished();
    setBusy(false);
}

static void
on_send_progress(struct netmd_send_progress *send_progress)
{
    QNetMDDevice *self = static_cast<QNetMDDevice *>(send_progress->user_data);

    self->onDownloadProgress(send_progress->progress);
}

void QNetMDDevice::onDownloadProgress(float progress)
{
    int progress_blocks = progress * download_total_file_blocks;
    downloadDialog.blockTransferred(progress_blocks - download_reported_file_blocks);
    download_reported_file_blocks = progress_blocks;

    QApplication::processEvents();
}

bool QNetMDDevice::download(const QString &filename)
{
    downloadDialog.starttrack(filename);

    download_reported_file_blocks = 0;
    download_total_file_blocks = QFileInfo(filename).size();

    int res = netmd_send_track(devh, filename.toUtf8().data(), NULL, on_send_progress, this);

    onDownloadProgress(1.f);

    if (res == NETMD_NO_ERROR) {
        downloadDialog.trackSucceeded();
        return true;
    }

    downloadDialog.trackFailed(QString("netmd_send_track() returned %1").arg(res));
    return false;
}

bool QNetMDDevice::isWritable()
{
    return netmd_is_disc_writable(devh);
}

bool QNetMDDevice::canFormatDisk()
{
    return isWritable();
}

bool QNetMDDevice::formatDisk()
{
    netmd_error error = netmd_erase_disc(devh);
    if (error != NETMD_NO_ERROR) {
        return false;
    }

    // This should commit the changes to disk
    error = netmd_secure_leave_session(devh);

    return error == NETMD_NO_ERROR;
}

QString
QNetMDDevice::getRecordedLabelText()
{
    return "Recorded: " + recordedLabelText;
}

QString
QNetMDDevice::getAvailableLabelText()
{
    return "Available: " + availableLabelText;
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
        if(uploadDialog.was_canceled())
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
        if(uploadDialog.was_canceled())
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
        if (uploadDialog.was_canceled()) {
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
        if(uploadDialog.was_canceled())
            break;
    }

    uploadDialog.finished();
    setBusy(false);
}

bool QHiMDDevice::download(const QString &filename)
{
    downloadDialog.starttrack(filename);

    int res = himd_writemp3(himd, filename.toUtf8().data());

    // Until we have a progress callback for the himd_writemp3() function,
    // just progress the whole file at once after the transfer is complete.
    downloadDialog.blockTransferred(QFile(filename).size());

    if (res == 0) {
        downloadDialog.trackSucceeded();
        return true;
    }

    downloadDialog.trackFailed(QString("himd_writemp3() returned %1").arg(res));
    return false;
}

bool QHiMDDevice::canUpload()
{
    // Every Hi-MD device has the possiblity to upload tracks in Hi-MD mode,
    // only the NetMD uploads are restricted to the MZ-RH1 (whether a single
    // track is uploadable or not due to DRM will be checked separately)
    return true;
}

QStringList QHiMDDevice::downloadableFileExtensions() const
{
    return QStringList() << "mp3";
}

bool QHiMDDevice::isWritable()
{
    // TODO: Check if it's mounted read-only
    return true;
}

bool QHiMDDevice::canFormatDisk()
{
    // Not implemented yet, see basictools/himdformat.c
    return false;
}

bool QHiMDDevice::formatDisk()
{
    // Not implemented yet, see basictools/himdformat.c
    return false;
}

QString
QHiMDDevice::getRecordedLabelText()
{
    return "Recorded: ...";
}

QString
QHiMDDevice::getAvailableLabelText()
{
    return "Available: ...";
}
