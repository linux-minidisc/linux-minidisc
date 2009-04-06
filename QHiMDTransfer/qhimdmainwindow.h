#ifndef QHIMDMAINWINDOW_H
#define QHIMDMAINWINDOW_H

#include <QtGui/QMainWindow>

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

private slots:
    void on_action_About_triggered();
};

#endif // QHIMDMAINWINDOW_H
