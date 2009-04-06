#include "qhimdmainwindow.h"
#include "ui_qhimdmainwindow.h"

QHiMDMainWindow::QHiMDMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::QHiMDMainWindowClass)
{
    ui->setupUi(this);
}

QHiMDMainWindow::~QHiMDMainWindow()
{
    delete ui;
}

void QHiMDMainWindow::on_action_About_triggered()
{

}
