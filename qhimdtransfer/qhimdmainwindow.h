#ifndef QHIMDMAINWINDOW_H
#define QHIMDMAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QtGui/QFileDialog>
#include <QSettings>
#include "qhimdaboutdialog.h"
#include "qhimdformatdialog.h"
#include "qhimduploaddialog.h"
#include "qhimdmodel.h"
#include "../libhimd/himd.h"
#include <tlist.h>
#include <fileref.h>
#include <tfile.h>
#include <tag.h>

extern "C" {
#include <sox.h>
}

namespace Ui
{
    class QHiMDMainWindowClass;
}

class QHiMDMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QHiMDMainWindow(QWidget *parent = 0);
    ~QHiMDMainWindow();

private:
    Ui::QHiMDMainWindowClass *ui;
    QHiMDAboutDialog * aboutDialog;
    QHiMDFormatDialog * formatDialog;
    QHiMDUploadDialog * uploadDialog;
    QHiMDTracksModel trackmodel;
    QString dumpmp3(const QHiMDTrack & trk, QString file);
    QString dumpoma(const QHiMDTrack & trk, QString file);
    QString dumppcm(const QHiMDTrack & trk, QString file);
    void checkfile(QString UploadDirectory, QString &filename, QString extension);
    void set_buttons_enable(bool connect, bool download, bool upload, bool rename, bool del, bool format, bool quit);

private slots:
    void on_action_Connect_triggered();
    void on_action_Format_triggered();
    void on_action_Upload_triggered();
    void on_action_Download_triggered();
    void on_action_Quit_triggered();
    void on_action_About_triggered();
};

#endif // QHIMDMAINWINDOW_H
