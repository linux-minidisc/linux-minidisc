TEMPLATE = app
CONFIG += link_prl link_pkgconfig
TARGET = qhimdtransfer
DEPENDPATH += .
INCLUDEPATH += .

# Input
TRANSLATIONS += qhimdtransfer_de.ts qhimdtransfer_nb.ts qhimdtransfer_se.ts \
                qhimdtransfer_fr.ts qhimdtransfer_pt.ts qhimdtransfer_pl.ts

HEADERS += qhimdaboutdialog.h \
           qhimdformatdialog.h \
           qhimduploaddialog.h \
           qhimdmainwindow.h \
           qhimdmodel.h
FORMS += qhimdaboutdialog.ui qhimdformatdialog.ui qhimduploaddialog.ui qhimdmainwindow.ui
SOURCES += main.cpp \
           qhimdaboutdialog.cpp \
           qhimdformatdialog.cpp \
           qhimduploaddialog.cpp \
           qhimdmainwindow.cpp \
           qhimdmodel.cpp
RESOURCES += icons.qrc
PKGCONFIG += sox taglib
win32: RC_FILE = qhimdtransfer.rc

# Mixed case version of the target name for operating systems on which
# this is convention.
win32: TARGET=QHiMDTransfer
mac: TARGET=QHiMDTransfer

include(../libhimd/use_libhimd.pri)
