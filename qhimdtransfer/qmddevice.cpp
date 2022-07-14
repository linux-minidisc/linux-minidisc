#include <qmddevice.h>
#include <QMessageBox>
#include <QApplication>
#include <QFile>
#include <QProgressDialog>
#include <QDir>
#include <QStorageInfo>
#include <QTemporaryFile>
#include <QProcess>
#include <QThread>
#include "wavefilewriter.h"
#include "himd.h"
#include "qmdutil.h"

#include <tlist.h>
#include <fileref.h>
#include <tfile.h>
#include <tag.h>
#include <libusb.h>

/* common device members */
QMDDevice::QMDDevice()
    : dev_type(NO_DEVICE)
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

void
QMDDevice::batchDownload(QHiMDUploadDialog *dialog, const QStringList &filenames)
{
    setBusy(true);

    for (const QString &filename: filenames) {
        dialog->addTask(-1, filename, QFileInfo(filename).size(), [this] (TransferTask &task) {
            download(task);
        });
    }

    dialog->exec();

    setBusy(false);
}

void
QMDDevice::batchUpload(QHiMDUploadDialog *dialog, QMDTrackIndexList tlist, QString path)
{
    setBusy(true);

    for (int trackIndex: tlist) {
        auto size = getTrackBlockCount(trackIndex);
        dialog->addTask(trackIndex, QString("Track %1").arg(trackIndex + 1), size, [this, path] (TransferTask &task) {
            upload(task, path);
        });
    }

    dialog->exec();

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
            .arg(isWritable() ? "writable" : "write-protected");

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
    // Basically everything that ffmpeg can convert, but let's just list a few
    // fan favorites here. As we don't support transferring ATRAC1 files
    // (.aea) without decompression, we don't list them here.
    return QStringList() << "wav" << "mp3" << "flac" << "ogg" << "m4a" << "oma";
}

static void
on_recv_progress(struct netmd_recv_progress *recv_progress)
{
    TransferTask *task = static_cast<TransferTask *>(recv_progress->user_data);

    task->progress(recv_progress->progress);
}

void QNetMDDevice::upload(TransferTask &task, QString path)
{
    int trackidx = task.trackIndex();

    QString oldcwd = QDir::currentPath();

    QDir::setCurrent(path);

    struct netmd_track_info info;
    netmd_error err = netmd_get_track_info(devh, trackidx, &info);

    if (err == NETMD_NO_ERROR) {
        char *filename = netmd_recv_get_default_filename(&info);

        // TODO: use checkfile() for auto-rename
        // TODO: Could report new filename

        err = netmd_recv_track(devh, trackidx, filename, on_recv_progress, &task);

        netmd_free_string(filename);
    }

    task.finish(err == NETMD_NO_ERROR, netmd_strerror(err));

    QDir::setCurrent(oldcwd);
}

static void
on_send_progress(struct netmd_send_progress *send_progress)
{
    TransferTask *task = static_cast<TransferTask *>(send_progress->user_data);

    task->progress(send_progress->progress);
}

void
QNetMDDevice::download(TransferTask &task)
{
    QString filename = task.filename();

    QString title = QFileInfo(filename).completeBaseName();

    QString fn = filename;

    // TODO: Properly check if we need to convert
    if (!filename.endsWith(".wav")) {
        // TODO: Could extract title from ID3 tags

        QTemporaryFile temp("mdupload.XXXXXX.wav");
        temp.open();

        int res = -1;

        if (filename.endsWith(".oma")) {
            // Assume ATRAC3 audio for OMA files, and just remux using ffmpeg
            // (TODO: parse header properly and deal with non-ATRAC3 data)
            res = QProcess::execute("ffmpeg", {"-i", filename, "-y", "-c:a", "copy", temp.fileName()});
        } else {
            res = QProcess::execute("ffmpeg", {"-i", filename, "-y", "-ar", "44100", "-ac", "2", "-c:a", "pcm_s16le", temp.fileName()});
        }

        if (res != 0) {
            // temp will be auto-removed here
            task.finish(false, QString("ffmpeg exited with status %1").arg(res));
            return;
        }

        // Keep the file around, we'll remove it after the transfer
        temp.setAutoRemove(false);
        fn = temp.fileName();
    }

    // TODO: Could convert to LP2/LP4 using atracdenc here

    netmd_error res = netmd_send_track(devh, fn.toUtf8().data(), title.toUtf8().data(), on_send_progress, &task);

    if (fn != filename) {
        QFile(fn).remove();
    }

    task.finish(res == NETMD_NO_ERROR, netmd_strerror(res));
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
    return recordedLabelText;
}

QString
QNetMDDevice::getAvailableLabelText()
{
    return availableLabelText;
}

void
QNetMDDevice::play()
{
    netmd_play(devh);
}

void
QNetMDDevice::pause()
{
    netmd_pause(devh);
}

void
QNetMDDevice::stop()
{
    netmd_stop(devh);
}

void
QNetMDDevice::startRewind()
{
    netmd_rewind(devh);
}

void
QNetMDDevice::startFastForward()
{
    netmd_fast_forward(devh);
}

void
QNetMDDevice::gotoPreviousTrack()
{
    netmd_track_previous(devh);
}

void
QNetMDDevice::gotoNextTrack()
{
    netmd_track_next(devh);
}

qlonglong
QNetMDDevice::getTrackBlockCount(int trackIndex)
{
    return netmdTrack(trackIndex).blockcount();
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

    int total = 0;
    for (unsigned int i=0; i<trk_count; ++i) {
        total += himdTrack(i).durationSeconds();
    }
    total_duration = QTime(0, 0).addSecs(total);

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

QString QHiMDDevice::discTitle()
{
    char *str = himd_get_disc_title(himd, NULL);
    QString result = QString::fromUtf8(str);
    himd_free(str);

    return result;
}

QHiMDTrack QHiMDDevice::himdTrack(unsigned int trkindex)
{
    return QHiMDTrack(himd, trkindex);
}

QString QHiMDDevice::dumpmp3(TransferTask &task, const QHiMDTrack &trk, QString file)
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

    int blocks_transferred = 0;
    while(himd_mp3stream_read_block(&str, &data, &len, NULL, &status) >= 0)
    {
        if(f.write((const char*)data,len) == -1)
        {
            errmsg = tr("Error writing audio data");
            goto clean;
        }

        blocks_transferred++;
        task.progress((float)blocks_transferred / (float)task.totalSize());
        if (task.wasCanceled()) {
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

static TagLib::FileRef
open_tag(const QString &filename)
{
#ifdef Q_OS_WIN
    return TagLib::FileRef(filename.toStdWString().c_str());
#else
    return TagLib::FileRef(filename.toUtf8().data());
#endif
}

static void
addid3tag(const QString &title, const QString &artist, const QString &album, const QString &filename)
{
    TagLib::FileRef f = open_tag(filename);

    TagLib::Tag *t = f.tag();
    t->setTitle(QStringToTString(title));
    t->setArtist(QStringToTString(artist));
    t->setAlbum(QStringToTString(album));
    f.file()->save();
}

QString QHiMDDevice::dumpoma(TransferTask &task, const QHiMDTrack &track, QString file)
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

    int blocks_transferred = 0;

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
        blocks_transferred++;
        task.progress((float)blocks_transferred / (float)task.totalSize());

        if (task.wasCanceled()) {
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

QString QHiMDDevice::dumppcm(TransferTask &task, const QHiMDTrack &track, QString file)
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

    int blocks_transferred = 0;
    while (himd_nonmp3stream_read_block(&str, &data, &len, NULL, &status) >= 0) {
        if (!waveFile.write_signed_big_endian(reinterpret_cast<const int16_t *>(data), len / 2)) {
            errmsg = tr("Error writing audio data");
            goto clean;
        }

        blocks_transferred++;
        task.progress((float)blocks_transferred / (float)task.totalSize());

        if (task.wasCanceled()) {
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

void QHiMDDevice::upload(TransferTask &task, QString path)
{
    int trackidx = task.trackIndex();

    QString filename, errmsg;

    QHiMDTrack track = himdTrack(trackidx);
    QString title = track.title();
    QString artist = track.artist();

    if (title.isEmpty()) {
        filename = tr("Track %1").arg(trackidx+1);
    } else if (artist.isEmpty()) {
        filename = title;
    } else {
        filename = artist + " - " + title;
    }

    if (!track.copyprotected()) {
        // TODO: Use an enum for the codec, not string
        QString codec = track.codecname();
        if (codec == "MPEG") {
            checkfile(path, filename, ".mp3");
            // TODO: Could set new filename
            errmsg = dumpmp3 (task, track, path + "/" + filename + ".mp3");
            if (errmsg.isNull()) {
                addid3tag(track.title(), track.artist(), track.album(), path + "/" +filename + ".mp3");
            }
        } else if (codec == "LPCM") {
            checkfile(path, filename, ".wav");
            // TODO: Could set new filename
            errmsg = dumppcm (task, track, path + "/" + filename + ".wav");
        } else if (codec == "AT3+" || codec == "AT3 ") {
            checkfile(path, filename, ".oma");
            // TODO: Could set new filename
            errmsg = dumpoma (task, track, path + "/" + filename + ".oma");
        }
    } else {
        errmsg = tr("upload disabled because of DRM encryption");
    }

    task.finish(errmsg.isNull(), errmsg);
}

void QHiMDDevice::download(TransferTask &task)
{
    QString filename = task.filename();

    TagLib::FileRef taglib_file = open_tag(filename);

    TagLib::Tag *tag = taglib_file.tag();

    QString title = TStringToQString(tag->title());
    QString artist = TStringToQString(tag->artist());
    QString album = TStringToQString(tag->album());

    task.progress(0.f);

    int res = himd_writemp3(himd, filename.toUtf8().data(),
            title.isEmpty() ? nullptr : title.toUtf8().data(),
            artist.isEmpty() ? nullptr : artist.toUtf8().data(),
            album.isEmpty() ? nullptr : album.toUtf8().data());

    task.progress(1.f);

    task.finish(res == 0, QString("himd_writemp3() returned %1").arg(res));
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
    // FIXME: Does not work properly yet: !QStorageInfo(device_path).isReadOnly();
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
    QStorageInfo info(device_path);

    auto recordedLabelText = QString("%1 in %2 tracks (%3% of %4 MiB used, %5)")
        .arg(util::formatDuration(total_duration))
        .arg(trk_count)
        .arg(int(100 * (float)(info.bytesTotal() - info.bytesFree()) / info.bytesTotal()))
        .arg(int(info.bytesTotal() / (1024 * 1024)))
        .arg(isWritable() ? "writable" : "write-protected");

    return recordedLabelText;
}

QString
QHiMDDevice::getAvailableLabelText()
{
    QStorageInfo info(device_path);

    auto availableLabelText = QString("%1 MiB (MP3: %2 @ 128k, ~ %3 @ 192k, ~ %4 @ 320k)")
        .arg(int(info.bytesFree() / (1024 * 1024)))
        .arg(util::formatDuration(QTime(0, 0).addSecs(info.bytesFree() / (128 * 1000 / 8))))
        .arg(util::formatDuration(QTime(0, 0).addSecs(info.bytesFree() / (192 * 1000 / 8))))
        .arg(util::formatDuration(QTime(0, 0).addSecs(info.bytesFree() / (320 * 1000 / 8))));

    return availableLabelText;
}

qlonglong
QHiMDDevice::getTrackBlockCount(int trackIndex)
{
    return himdTrack(trackIndex).blockcount();
}
