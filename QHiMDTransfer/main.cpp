#include <QtGui/QApplication>
#include "qhimdmainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QHiMDMainWindow w;
    w.show();
    return a.exec();
}
