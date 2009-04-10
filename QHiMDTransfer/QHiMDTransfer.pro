# -------------------------------------------------
# Project created by QtCreator 2009-03-26T21:43:23
# -------------------------------------------------
TARGET = QHiMDTransfer
TEMPLATE = app
SOURCES += main.cpp \
    qhimdmainwindow.cpp \
    qhimdaboutdialog.cpp \
    qhimdformatdialog.cpp \
    qtc-gdbmacros/gdbmacros.cpp
HEADERS += qhimdmainwindow.h \
    qhimdaboutdialog.h \
    qhimdaboutdialog.h \
    qhimdformatdialog.h
LIBS += ../libhimd/libhimd.a
INCLUDEPATH = ..
FORMS += qhimdmainwindow.ui \
    qhimdaboutdialog.ui \
    qhimdformatdialog.ui
CONFIG += link_pkgconfig
PKGCONFIG += glib-2.0
