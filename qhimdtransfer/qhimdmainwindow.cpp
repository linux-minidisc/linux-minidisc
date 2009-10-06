#include "qhimdmainwindow.h"
#include "ui_qhimdmainwindow.h"
#include "qhimdaboutdialog.h"
#include "qmessagebox.h"

#include "sony_oma.h"

void QHiMDMainWindow::dumpmp3(struct himd * himd, int trknum, QString file)
{
    struct himd_mp3stream str;
    struct himderrinfo status;
    unsigned int len;
    const unsigned char * data;
    QFile f(file);

    if(!f.open(QIODevice::ReadWrite))
    {
        perror("Error opening file for MP3-output");
        return;
    }
    if(himd_mp3stream_open(himd, trknum, &str, &status) < 0)
    {
        fprintf(stderr, "Error opening track %d: %s\n", trknum, status.statusmsg);
        return;
    }
    while(himd_mp3stream_read_frame(&str, &data, &len, &status) >= 0)
    {
        if(f.write((const char*)data,len) == -1)
        {
            perror("writing dumped stream");
            goto clean;
        }
    }
    if(status.status != HIMD_STATUS_AUDIO_EOF)
        fprintf(stderr,"Error reading MP3 data: %s\n", status.statusmsg);

clean:
    f.close();
    himd_mp3stream_close(&str);
}

static inline TagLib::String QStringToTagString(const QString & s)
{
    return TagLib::String(s.toUtf8().data(), TagLib::String::UTF8);
}

void QHiMDMainWindow::addid3tag(QString title, QString artist, QString album, QString file)
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

void QHiMDMainWindow::dumpoma(struct himd * himd, int trknum, QString file)
{
    struct himd_nonmp3stream str;
    struct himderrinfo status;
    struct trackinfo trkinf;
    unsigned int len;
    const unsigned char * data;
    char header[EA3_FORMAT_HEADER_SIZE];
    QFile f(file);

    if(!f.open(QIODevice::ReadWrite))
    {
        perror("Error opening file for MP3-output");
        return;
    }
    if(himd_get_track_info(himd, trknum, &trkinf, &status) < 0)
    {
        fprintf(stderr, "Can't get track info for track %d: %s\n", trknum, status.statusmsg);
        return;
    }
    if(himd_nonmp3stream_open(himd, trknum, &str, &status) < 0)
    {
        fprintf(stderr, "Error opening track %d: %s\n", trknum, status.statusmsg);
        return;
    }

    make_ea3_format_header(header, &trkinf);
    if(f.write(header, sizeof header) == -1)
    {
        perror("writing header");
        goto clean;
    }
    while(himd_nonmp3stream_read_frame(&str, &data, &len, &status) >= 0)
    {
        if(f.write((const char*)data,len) == -1)
        {
            perror("writing dumped stream");
            goto clean;
        }
    }
    if(status.status != HIMD_STATUS_AUDIO_EOF)
        fprintf(stderr,"Error reading MP3 data: %s\n", status.statusmsg);

clean:
    f.close();
    himd_nonmp3stream_close(&str);
}

void QHiMDMainWindow::dumppcm(struct himd * himd, int trknum, QString file)
{
    struct himd_nonmp3stream str;
    struct himderrinfo status;
    unsigned int len, i;
    int left, right;
    int clipcount;
    const unsigned char * data;
    sox_format_t * out;
    sox_sample_t soxbuf [HIMD_MAX_PCMFRAME_SAMPLES * 2];
    sox_signalinfo_t signal_out;

    signal_out.channels = 2;
    signal_out.length = 0;
    signal_out.precision = 16;
    signal_out.rate = 44100;

    if(!(out = sox_open_write(file.toUtf8(), &signal_out, NULL, NULL, NULL, NULL)))
    {
        perror("Error opening file for LPCM-output");
        return;
    }
    if(himd_nonmp3stream_open(himd, trknum, &str, &status) < 0)
    {
        fprintf(stderr, "Error opening track %d: %s\n", trknum, status.statusmsg);
        return;
    }
    while(himd_nonmp3stream_read_frame(&str, &data, &len, &status) >= 0)
    {
      
      for(i = 0; i < len/4; i++) {

        left = data[i*4]*256+data[i*4+1];
        right = data[i*4+2]*256+data[i*4+3];
        if (left > 0x8000) left -= 0x10000;
        if (right > 0x8000) right -= 0x10000;

        soxbuf[i*2] = SOX_SIGNED_16BIT_TO_SAMPLE(left, clipcount);
        soxbuf[i*2+1] = SOX_SIGNED_16BIT_TO_SAMPLE(right, clipcount);
      }

      if (sox_write(out, soxbuf, len/2) == -1)
        {
            perror("writing dumped stream");
            goto clean;
        }
    }
    if(status.status != HIMD_STATUS_AUDIO_EOF)
        fprintf(stderr,"Error reading PCM data: %s\n", status.statusmsg);
clean:
    sox_close(out);
    himd_nonmp3stream_close(&str);
}

QString get_locale_str(struct himd * himd, int idx)
{
    QString outstr;
    char * str;
    str = himd_get_string_utf8(himd, idx, NULL, NULL);
    if(!str)
        return NULL;

    outstr = QString::fromUtf8(str);
    himd_free(str);
    return outstr;
}

void QHiMDMainWindow::checkfile(QString UploadDirectory, QString &filename, QString extension)
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

QHiMDMainWindow::QHiMDMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::QHiMDMainWindowClass)
{
    aboutDialog = new QHiMDAboutDialog;
    formatDialog = new QHiMDFormatDialog;
    ui->setupUi(this);
}

QHiMDMainWindow::~QHiMDMainWindow()
{
    delete ui;
}

/* Slots for the actions */

void QHiMDMainWindow::on_action_Download_triggered()
{
     QStringList DownloadFileList;


    DownloadFileList = QFileDialog::getOpenFileNames(
                         this,
                         "Select MP3s for download",
                         "/",
                         "MP3-files (*.mp3)");

}

void QHiMDMainWindow::on_action_Upload_triggered()
{
    QString UploadDirectory;
    QList<QTreeWidgetItem *> tracks;
    int i;
    QString filename;

    UploadDirectory = QFileDialog::getExistingDirectory(this,
                                                 "Select directory for Upload",
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    tracks = ui->TrackList->selectedItems();

    for(i = 0; i < tracks.size(); i++) {
        if(tracks[i]->text(1) == 0)
            filename = QString("Track ")+tracks[i]->text(0);
        else
            filename = tracks[i]->text(2)+QString(" - ")+tracks[i]->text(1);

        if (tracks[i]->text(5) == "MPEG")
        {
            checkfile(UploadDirectory, filename, ".mp3");
            dumpmp3 (&this->HiMD, tracks[i]->text(0).toInt(), UploadDirectory+QString("/")+filename+QString(".mp3"));
            addid3tag (tracks[i]->text(1),tracks[i]->text(2),tracks[i]->text(3), UploadDirectory+QString("/")+filename+QString(".mp3"));
        }
        else if (tracks[i]->text(5) == "LPCM")
        {
            checkfile(UploadDirectory, filename, ".wav");
            dumppcm (&this->HiMD, tracks[i]->text(0).toInt(), UploadDirectory+QString("/")+filename+QString(".wav"));
        }
        else if (tracks[i]->text(5) == "AT3+" || tracks[i]->text(5) == "AT3 ")
        {
            checkfile(UploadDirectory, filename, ".oma");
            dumpoma (&this->HiMD, tracks[i]->text(0).toInt(), UploadDirectory+QString("/")+filename+QString(".oma"));
        }
    }
}

void QHiMDMainWindow::on_action_Quit_triggered()
{
    this->close();
}

void QHiMDMainWindow::on_action_About_triggered()
{
    aboutDialog->show();
}

void QHiMDMainWindow::on_action_Format_triggered()
{
    formatDialog->show();
}

void QHiMDMainWindow::on_action_Connect_triggered()
{
    QString HiMDDirectory;
    QTreeWidgetItem * HiMDTrack;
    QString TrackNr;
    QMessageBox himdStatus;

    HiMDDirectory = QFileDialog::getExistingDirectory(this,
                                                 "Select directory of HiMD Medium",
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    ui->TrackList->clear();

    if (himd_open(&this->HiMD, (HiMDDirectory.toAscii()).data(), NULL)) {
        himdStatus.setText("Error opening HiMD-data. Make you sure, you chose the proper root-directory of your HiMD-Walkman.");
        himdStatus.exec();
        return;
    }

    for(int i = HIMD_FIRST_TRACK;i <= HIMD_LAST_TRACK;i++)
    {
        struct trackinfo t;
        HiMDTrack = new QTreeWidgetItem(0);
        if(himd_get_track_info(&this->HiMD, i, &t, NULL)  >= 0)
        {
            HiMDTrack->setText(0, TrackNr.setNum(i));
            HiMDTrack->setText(1, get_locale_str(&this->HiMD, t.title));
            HiMDTrack->setText(2, get_locale_str(&this->HiMD, t.artist));
            HiMDTrack->setText(3, get_locale_str(&this->HiMD, t.album));
            HiMDTrack->setText(4, QString("%1:%2").arg(t.seconds/60).arg(t.seconds%60,2,10,QLatin1Char('0')));
            HiMDTrack->setText(5, himd_get_codec_name(&t));
            HiMDTrack->setFlags(HiMDTrack->flags() |Qt::ItemIsEnabled);

            ui->TrackList->addTopLevelItem(HiMDTrack);
        }
    }

    ui->TrackList->update();
}
