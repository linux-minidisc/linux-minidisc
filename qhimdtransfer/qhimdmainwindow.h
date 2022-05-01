#ifndef QHIMDMAINWINDOW_H
#define QHIMDMAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QSettings>
#include "qhimdaboutdialog.h"
#include "qhimdformatdialog.h"
#include "qhimddetection.h"
#include "qmdmodel.h"

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
    QHiMDDetection * detect;
    QNetMDTracksModel ntmodel;
    QHiMDTracksModel htmodel;
    QSettings settings;
    QMDDevice * current_device;

    void set_buttons_enable();
    void init_himd_browser(QMDTracksModel *model);
    void save_window_settings();
    void read_window_settings();
    bool autodetect_init();
    void setCurrentDevice(QMDDevice * dev);
    void open_device(QMDDevice * dev);
    void upload_to(const QString & path);

private slots:
    void on_action_Connect_triggered();
    void on_action_Format_triggered();
    void on_action_Upload_triggered();
    void on_action_Download_triggered();
    void on_action_Quit_triggered();
    void on_action_About_triggered();
    void on_action_Help_triggered();
    void on_action_Rename_triggered();
    void on_action_Delete_triggered();
    void handle_himd_selection_change(const QItemSelection&, const QItemSelection&);
    void device_list_changed(QMDDevicePtrList dplist);
    void on_himd_devices_activated(QString device);
    void current_device_closed();
};

#endif // QHIMDMAINWINDOW_H
