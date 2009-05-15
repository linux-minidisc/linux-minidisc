#ifndef QHIMDMAINWINDOW_H
#define QHIMDMAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QFileDialog>
#include "qhimdaboutdialog.h"
#include "qhimdformatdialog.h"
#include "../libhimd/himd.h"

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

private:
    Ui::QHiMDMainWindowClass *ui;
    QHiMDAboutDialog * aboutDialog;
    QHiMDFormatDialog * formatDialog;
    void dumpmp3(struct himd * himd, int trknum, QString file);
    void dumppcm(struct himd * himd, int trknum, QString file);

private slots:
    void on_action_Connect_triggered();
    void on_action_Format_triggered();
    void on_action_Upload_triggered();
    void on_action_Download_triggered();
    void on_action_Quit_triggered();
    void on_action_About_triggered();
};

#endif // QHIMDMAINWINDOW_H
