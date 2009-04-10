#ifndef QHIMDMAINWINDOW_H
#define QHIMDMAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QFileDialog>
#include "qhimdaboutdialog.h"
#include "qhimdformatdialog.h"
extern "C" {
#include "libhimd/himd.h"
}

namespace Ui
{
    class QHiMDMainWindowClass;
}

class QHiMDMainWindow : public QMainWindow
{
    Q_OBJECT

    struct himd HiMD;

public:
    QHiMDMainWindow(QWidget *parent = 0);
    ~QHiMDMainWindow();

public slots:
    void on_trigger_Connect();
    void on_trigger_Download();
    void on_trigger_Upload();
    void on_trigger_Delete();
    void on_trigger_Rename();
    void on_trigger_Format();
    void on_trigger_AddGroup();
    void on_trigger_Quit();

private:
    Ui::QHiMDMainWindowClass *ui;
    QHiMDAboutDialog * aboutDialog;
    QHiMDFormatDialog * formatDialog;

private slots:
    void on_action_Format_triggered();
    void on_action_Upload_triggered();
    void on_action_Download_triggered();
    void on_action_Quit_triggered();
    void on_action_About_triggered();
};

#endif // QHIMDMAINWINDOW_H
