#ifndef QHIMDMAINWINDOW_H
#define QHIMDMAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QFileSystemModel>
#include "qhimdaboutdialog.h"
#include "qhimdformatdialog.h"
#include "qhimduploaddialog.h"
#include "qhimddetection.h"
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
    QHiMDDetection * detect;
    QHiMDTracksModel trackmodel;
    QFileSystemModel localmodel;
    QSettings settings;
    QString dumpmp3(const QHiMDTrack & trk, QString file);
    QString dumpoma(const QHiMDTrack & trk, QString file);
    QString dumppcm(const QHiMDTrack & trk, QString file);
    void checkfile(QString UploadDirectory, QString &filename, QString extension);
    void set_buttons_enable(bool connect, bool download, bool upload, bool rename, bool del, bool format, bool quit);
    void init_himd_browser();
    void init_local_browser();
    bool autodetect_init();
    void open_himd_at(const QString & path);
    void upload_to(const QString & path);

private slots:
    void on_action_Connect_triggered();
    void on_action_Format_triggered();
    void on_action_Upload_triggered();
    void on_action_Download_triggered();
    void on_action_Quit_triggered();
    void on_action_About_triggered();
    void on_localScan_clicked(QModelIndex index);
    void on_upload_button_clicked();
    void handle_selection_change(const QItemSelection&, const QItemSelection&);
    void himd_found(QString path);
    void himd_removed(QString path);
    void on_himd_devices_activated(QString device);

signals:
    void himd_busy(QString path);
    void himd_idle(QString path);
};

#endif // QHIMDMAINWINDOW_H
