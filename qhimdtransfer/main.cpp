#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include "qhimdmainwindow.h"

/* stolen from Qt Creator */
#ifdef Q_OS_MAC
#define SHARE_FROM_BIN "/../Resources"
#else
#define SHARE_FROM_BIN "/../share/qhimdtransfer"
#endif

#include "libnetmd.h"

int main(int argc, char *argv[])
{
    int status;

    for (int i=1; i<argc; ++i) {
        if (strcmp(argv[i], "-v") == 0) {
            netmd_set_log_level(NETMD_LOG_ALL);
        }
    }

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
    return status;
}
