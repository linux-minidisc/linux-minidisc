#include <QtGui/QApplication>
#include "qhimdmainwindow.h"

int main(int argc, char *argv[])
{
    int status;
    sox_format_init();
    QApplication a(argc, argv);
    QHiMDMainWindow w;
    w.show();
    status = a.exec();
    sox_format_quit();
    return status;
}
