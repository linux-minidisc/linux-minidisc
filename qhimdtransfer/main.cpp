#include <QApplication>
#include <QtCore/QTranslator>
#include <QtCore/QLocale>
#include "qhimdmainwindow.h"

/* stolen from Qt Creator */
#ifdef Q_OS_MAC
#define SHARE_FROM_BIN "/../Resources"
#else
#define SHARE_FROM_BIN "/../share/qhimdtransfer"
#endif

int main(int argc, char *argv[])
{
    int status;
    sox_format_init();

    QApplication a(argc, argv);
    QTranslator trans;
    QString transfile = QString("qhimdtransfer_") + QLocale::system().name();
    QString transdir = QCoreApplication::applicationDirPath() +
                         QString(SHARE_FROM_BIN "/translations");
    // try cwd, then standard translation directory
    trans.load(transfile) || trans.load(transfile, transdir);
    a.installTranslator(&trans);
    a.setOrganizationName("linux-minidisc");
    a.setApplicationName("QHiMDTransfer");

    QHiMDMainWindow w;
    w.show();
    status = a.exec();
    sox_format_quit();
    return status;
}
