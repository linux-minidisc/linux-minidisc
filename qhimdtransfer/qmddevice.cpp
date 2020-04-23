#include <qmddevice.h>
#include <QMessageBox>
#include <QApplication>
#include <QFile>
#include "wavefilewriter.h"
#include "himd.h"

#include <tlist.h>
#include <fileref.h>
#include <tfile.h>
#include <tag.h>

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

    netmd_initialize_disc_info(devh, &current_md);

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

    netmd_clean_disc_info(&current_md);
    netmd_close(devh);
    devh = NULL;

    is_open = false;
    trk_count = 0;
    md_inserted = false;
    emit closed();
}

QString QNetMDDevice::discTitle()
{
    return QString(current_md.groups[0].name);
}

QNetMDTrack QNetMDDevice::netmdTrack(unsigned int trkindex)
{
    minidisc * disc = &current_md;

    return QNetMDTrack(devh, disc, trkindex);
}

QString QNetMDDevice::upload_track_blocks(uint32_t length, FILE *file, size_t chunksize)
{
    /* this is a copy of netmd_secure_real_recv_track(...) function, but updates upload dialog progress bar */
    uint32_t done = 0;
    unsigned char *data;
    int status;
    netmd_error error = NETMD_NO_ERROR;
    int transferred = 0;

    data = (unsigned char *)malloc(chunksize);
    while (done < length) {
        if ((length - done) < chunksize) {
            chunksize = length - done;
        }

        status = libusb_bulk_transfer((libusb_device_handle*)devh, 0x81, data, (int)chunksize, &transferred, 10000);

        if (status >= 0) {
            done += transferred;
            fwrite(data, transferred, 1, file);
            netmd_log(NETMD_LOG_DEBUG, "%.1f%%\n", (double)done/(double)length * 100);

            uploadDialog.blockTransferred();
            QApplication::processEvents();
            /* do not check for uploadDialog.upload_canceled() here, netmd device will remain busy if track upload hasn´t finished */
        }
        else if (status != -LIBUSB_ERROR_TIMEOUT) {
            error = NETMD_USB_ERROR;
        }
    }
    free(data);

    return (error != NETMD_NO_ERROR) ? netmd_strerror(error) : QString();
}

void QNetMDDevice::upload(unsigned int trackidx, QString path)
{
    /* this is a copy of netmd_secure_recv_track(...) function, we need single block transfer function to make use of a progress bar,
     * maybe we can add/change something inside libnetmd for this
     */
    QNetMDTrack track = netmdTrack(trackidx);
    uint16_t track_id = trackidx;
    unsigned char cmdhdr[] = {0x00, 0x10, 0x01};
    unsigned char cmd[sizeof(cmdhdr) + sizeof(track_id)] = { 0 };
    unsigned char *buf;
    unsigned char codec;
    uint32_t length;
    netmd_response response;
    netmd_error error;
    QString filename, errmsg, filepath;
    FILE * file = NULL;

    if(name() != "SONY MZ-RH1 (NetMD)")
    {
        errmsg = tr("upload disabled, %1 does not support netmd track uploads").arg(name());
        goto clean;
    }

    if(track.copyprotected())
    {
        errmsg = tr("upload disabled, Track is copy protected");
        goto clean;
    }

    // create filename first
    if(track.title().isEmpty())
        filename = tr("Track %1").arg(track.tracknum() + 1);
    else
        filename = track.title();

    if(track.bitrate_id == NETMD_ENCODING_SP) {
        checkfile(path, filename, ".aea");
        filepath = path + "/" + filename + ".aea";
    }
    else {
        checkfile(path, filename, ".wav");
        filepath = path + "/" + filename + ".wav";
    }

    if(!(file = fopen(filepath.toUtf8().data(), "wb"))) {
            errmsg = tr("cannot open file %1 for writing").arg(filepath);
            goto clean;
    }

    buf = cmd;
    memcpy(buf, cmdhdr, sizeof(cmdhdr));
    buf += sizeof(cmdhdr);
    netmd_copy_word_to_buffer(&buf, trackidx + 1U, 0);

    netmd_send_secure_msg(devh, 0x30, cmd, sizeof(cmd));
    error = netmd_recv_secure_msg(devh, 0x30, &response, NETMD_STATUS_INTERIM);
    netmd_check_response_bulk(&response, cmdhdr, sizeof(cmdhdr), &error);
    netmd_check_response_word(&response, track_id + 1U, &error);
    codec = netmd_read(&response);
    length = netmd_read_doubleword(&response);

    /* initialize track.blockcount() here, needed by progress bar in the uploadDialog */
    track.setBlocks(length%NETMD_RECV_BUF_SIZE ? length / NETMD_RECV_BUF_SIZE + 1 : length / NETMD_RECV_BUF_SIZE);
    uploadDialog.starttrack(track, filename);
    if (track.bitrate_id == NETMD_ENCODING_SP) {
        netmd_write_aea_header(track.title().toUtf8().data(), codec, track.channel, file);
    }
    else {
        netmd_write_wav_header(codec, length, file);
    }

    errmsg = upload_track_blocks(length, file, NETMD_RECV_BUF_SIZE);
    if(!errmsg.isNull()) {
        goto clean;
    }

    error = netmd_recv_secure_msg(devh, 0x30, &response, NETMD_STATUS_ACCEPTED);
    netmd_check_response_bulk(&response, cmdhdr, sizeof(cmdhdr), &error);
    netmd_read_response_bulk(&response, NULL, 2, &error);
    netmd_check_response_word(&response, 0, &error);

    if(error != NETMD_NO_ERROR)
        errmsg = QString(netmd_strerror(error));

clean:
    if(errmsg.isNull())
        uploadDialog.trackSucceeded();
    else
        uploadDialog.trackFailed(errmsg);

    if(file)
        fclose(file);
    if(!errmsg.isNull())
    {
        QFile f(filepath);
        if(f.exists())
            f.remove();
    }
}

void QNetMDDevice::batchUpload(QMDTrackIndexList tlist, QString path)
{
    int allblocks = 0;

    setBusy(true);

    /* progress bar for all tracks does not work yet, is there any way to get track length without recieving a complete track ?
     * as far as i´ve tested device remains busy if download procedure hasn´t finished.
     * progressbar for all tracks shows idle mode if maximum value is set to 0
     */
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

bool QNetMDDevice::download(const QString &filename)
{
    Q_UNUSED(filename);
    // TODO: Implement support for downloading data to NetMD device
    return false;
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
