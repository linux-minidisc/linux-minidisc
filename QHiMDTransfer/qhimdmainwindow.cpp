#include "qhimdmainwindow.h"
#include "ui_qhimdmainwindow.h"
#include "qhimdaboutdialog.h"

QString get_locale_str(struct himd * himd, int idx)
{
    char * str;
    QString outstr;
    str = himd_get_string_utf8(himd, idx, NULL);
    if(!str)
        return NULL;

    outstr = g_locale_from_utf8(str,-1,NULL,NULL,NULL);
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

/* Slots for the buttons */

void QHiMDMainWindow::on_trigger_Connect()
{
    QString HiMDDirectory;
    QTreeWidgetItem * HiMDTrack;// = new QTreeWidgetItem(0);;

    HiMDDirectory = QFileDialog::getExistingDirectory(this,
                                                 "Select directory of HiMD Medium",
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    himd_open(&this->HiMD, (HiMDDirectory.toAscii()).data());

    for(int i = HIMD_FIRST_TRACK;i <= HIMD_LAST_TRACK;i++)
    {
        struct trackinfo t;
        HiMDTrack = new QTreeWidgetItem(0);
        if(himd_get_track_info(&this->HiMD, i, &t)  >= 0)
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

            HiMDTrack->setText(0, get_locale_str(&this->HiMD, t.title));
            HiMDTrack->setText(1, get_locale_str(&this->HiMD, t.artist));
            HiMDTrack->setText(2, get_locale_str(&this->HiMD, t.album));
            HiMDTrack->setText(3, "(unset)");
            HiMDTrack->setText(4, "(unset)");
            HiMDTrack->setFlags(Qt::ItemIsEnabled);

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

void QHiMDMainWindow::on_trigger_Download()
{
    QStringList DownloadFileList;

    DownloadFileList = QFileDialog::getOpenFileNames(
                         this,
                         "Select MP3s for download",
                         "/",
                         "MP3-files (*.mp3)");
}

void QHiMDMainWindow::on_trigger_Upload()
{
    QString UploadDirectory;

    UploadDirectory = QFileDialog::getExistingDirectory(this,
                                                 "Select directory for Upload",
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
}

void QHiMDMainWindow::on_trigger_Delete()
{

}

void QHiMDMainWindow::on_trigger_Rename()
{

}

void QHiMDMainWindow::on_trigger_Format()
{
    formatDialog->show();
}

void QHiMDMainWindow::on_trigger_AddGroup()
{

}

void QHiMDMainWindow::on_trigger_Quit()
{
    this->close();
}

/* Slots for the menu-items */

void QHiMDMainWindow::on_action_Download_triggered()
{
    this->on_trigger_Download();
}

void QHiMDMainWindow::on_action_Upload_triggered()
{
    this->on_trigger_Upload();
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
    this->on_trigger_Format();
}
