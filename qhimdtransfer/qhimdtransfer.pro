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

include(../libhimd/use_libhimd.pri)