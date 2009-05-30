#include <QtGui/QApplication>
#include "qhimdmainwindow.h"

int main(int argc, char *argv[])
{
    sox_format_init();
    QApplication a(argc, argv);
    QHiMDMainWindow w;
    w.show();
    return a.exec();
}
