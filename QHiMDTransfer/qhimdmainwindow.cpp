#include "qhimdmainwindow.h"
#include "ui_qhimdmainwindow.h"
#include "qhimdaboutdialog.h"
#include "qmessagebox.h"

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

void QHiMDMainWindow::dumppcm(struct himd * himd, int trknum, QString file)
{
    struct himd_pcmstream str;
    struct himderrinfo status;
    unsigned int len, i;
    int left, right;
    int clipcount;
    const unsigned char * data;
    //    QFile f(file);
    sox_format_t * out;
    sox_sample_t soxbuf [HIMD_MAX_PCMFRAME_SAMPLES * 2];
    sox_signalinfo_t signal_out;

    signal_out.channels = 2;
    signal_out.length = 0;
    signal_out.precision = 16;
    signal_out.rate = 44100;

    //    if(!f.open(QIODevice::ReadWrite))
    if(!(out = sox_open_write(file.toUtf8(), &signal_out, NULL, NULL, NULL, NULL)))
    {
        perror("Error opening file for LPCM-output");
        return;
    }
    if(himd_pcmstream_open(himd, trknum, &str, &status) < 0)
    {
        fprintf(stderr, "Error opening track %d: %s\n", trknum, status.statusmsg);
        return;
    }
    while(himd_pcmstream_read_frame(&str, &data, &len, &status) >= 0)
    {
      
      for(i = 0; i < len/4; i++) {

        left = data[i*4]*256+data[i*4+1];
        right = data[i*4+2]*256+data[i*4+3];
        if (left > 0x8000) left -= 0x10000;
        if (right > 0x8000) right -= 0x10000;

        soxbuf[i*2] = SOX_SIGNED_16BIT_TO_SAMPLE(left, clipcount);
        soxbuf[i*2+1] = SOX_SIGNED_16BIT_TO_SAMPLE(right, clipcount);
      }

      //         if(f.write((const char*)data,len) == -1)
      if (sox_write(out, soxbuf, len/2) == -1)
        {
            perror("writing dumped stream");
            goto clean;
        }
    }
    if(status.status != HIMD_STATUS_AUDIO_EOF)
        fprintf(stderr,"Error reading PCM data: %s\n", status.statusmsg);
clean:
    //    f.close();
    sox_close(out);
    himd_pcmstream_close(&str);
}

// void QHiMDMainWindow::pcmaddheader(QString file) {

//     struct sox_format_t inFormat, outFormat;
//     struct sox_encodinginfo_t inEncoding, outEncoding;
//     struct sox_signalinfo_t inInfo;

//     inInfo.channels = 2;
//     inInfo.length = 0;
//     inInfo.precision = 16;
//     inInfo.rate = 44100;

//     inEncoding.opposite_endian = sox_true;

//     sox_format_init();

//     inFormat = sox_open_read(file.latin1(), inInfo, inEncoding, "RAW");

//     outFormat = sox_open_write(file.latin1(), &inFormat->signal, NULL, NULL, NULL, NULL);

//     sox_format_quit();
// }

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

    UploadDirectory = QFileDialog::getExistingDirectory(this,
                                                 "Select directory for Upload",
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    tracks = ui->TrackList->selectedItems();

    for(i = 0; i < tracks.size(); i++) {
        if (tracks[i]->text(5) == "MPEG")
            dumpmp3 (&this->HiMD, tracks[i]->text(0).toInt(), UploadDirectory+QString("/")+tracks[i]->text(2)+QString(" - ")+tracks[i]->text(1)+QString(".mp3"));
        else if (tracks[i]->text(5) == "LPCM")
            dumppcm (&this->HiMD, tracks[i]->text(0).toInt(), UploadDirectory+QString("/")+tracks[i]->text(2)+QString(" - ")+tracks[i]->text(1)+QString(".wav"));
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
    QTreeWidgetItem * HiMDTrack;// = new QTreeWidgetItem(0);;
    QString TrackNr;

    HiMDDirectory = QFileDialog::getExistingDirectory(this,
                                                 "Select directory of HiMD Medium",
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    ui->TrackList->clear();

    himd_open(&this->HiMD, (HiMDDirectory.toAscii()).data(), NULL);

    for(int i = HIMD_FIRST_TRACK;i <= HIMD_LAST_TRACK;i++)
    {
        struct trackinfo t;
        HiMDTrack = new QTreeWidgetItem(0);
        if(himd_get_track_info(&this->HiMD, i, &t, NULL)  >= 0)
        {
        //    char * title = get_locale_str(himd, t.title);
        //    char * artist = get_locale_str(himd, t.artist);
        //    char * album = get_locale_str(himd, t.album);
        //    printf("%4d: %d:%02d %s %s:%s (%s %d)\n",
        //          i, t.seconds/60, t.seconds % 60, codecstr(&t),
        //            artist, title, album, t.trackinalbum);
     //       g_free(title);
       //     g_free(artist);
         //   g_free(album);

            HiMDTrack->setText(0, TrackNr.setNum(i));
            HiMDTrack->setText(1, get_locale_str(&this->HiMD, t.title));
            HiMDTrack->setText(2, get_locale_str(&this->HiMD, t.artist));
            HiMDTrack->setText(3, get_locale_str(&this->HiMD, t.album));
            HiMDTrack->setText(4, QString("%1:%2").arg(t.seconds/60).arg(t.seconds%60,2,10,QLatin1Char('0')));
            HiMDTrack->setText(5, himd_get_codec_name(&t));
            HiMDTrack->setFlags(HiMDTrack->flags() |Qt::ItemIsEnabled);

            ui->TrackList->addTopLevelItem(HiMDTrack);
/*
            if(verbose)
            {
                struct fraginfo f;
                int fnum = t.firstfrag;
                while(fnum != 0)
                {
                    if(himd_get_fragment_info(himd, fnum, &f) >= 0)
                    {
                        printf("     %3d@%05d .. %3d@%05d\n", f.firstframe, f.firstblock, f.lastframe, f.lastblock);
                        fnum = f.nextfrag;
                    }
                    else
                    {
                        printf("     ERROR reading fragment %d info: %s\n", fnum, himd->statusmsg);
                        break;
                    }
                }

            }
*/
        }
    }

    ui->TrackList->update();
}
