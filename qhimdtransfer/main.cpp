#include <QtGui/QApplication>
#include <QtCore/QTranslator>
#include <QtCore/QLocale>
#include "qhimdmainwindow.h"

int main(int argc, char *argv[])
{
    int status;
    sox_format_init();
    
    QApplication a(argc, argv);
    QTranslator trans;
    trans.load(QString("qhimdtransfer_") + QLocale::system().name());
    a.installTranslator(&trans);
    a.setOrganizationName("linux-minidisc");
    a.setApplicationName("QHiMDTransfer");
    
    QHiMDMainWindow w;
    w.show();
    status = a.exec();
    sox_format_quit();
    return status;
}
