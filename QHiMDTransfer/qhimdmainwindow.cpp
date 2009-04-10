#include "qhimdmainwindow.h"
#include "ui_qhimdmainwindow.h"
#include "qhimdaboutdialog.h"

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

    HiMDDirectory = QFileDialog::getExistingDirectory(this,
                                                 "Select directory of HiMD Medium",
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);
    himd_open(&this->HiMD, (HiMDDirectory.toAscii()).data());
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
