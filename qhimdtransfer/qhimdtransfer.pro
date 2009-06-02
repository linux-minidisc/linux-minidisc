TEMPLATE = app
CONFIG += link_prl link_pkgconfig 
TARGET = qhimdtransfer
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += qhimdaboutdialog.h \
           qhimdformatdialog.h \
           qhimdmainwindow.h 
FORMS += qhimdaboutdialog.ui qhimdformatdialog.ui qhimdmainwindow.ui
SOURCES += main.cpp \
           qhimdaboutdialog.cpp \
           qhimdformatdialog.cpp \
           qhimdmainwindow.cpp
RESOURCES += icons.qrc
PKGCONFIG += sox
win32: RC_FILE = qhimdtransfer.rc

# Mixed case version of the target name for operating systems on which
# this is convention.
win32: TARGET=QHiMDTransfer
mac: TARGET=QHiMDTransfer

include(../libhimd/use_libhimd.pri)
