TEMPLATE = app
CONFIG += link_prl link_pkgconfig

# Mixed case target name for operating systems on which this is convention
win32|mac {
    TARGET = QHiMDTransfer
} else {
    TARGET = qhimdtransfer
}

DEPENDPATH += .
INCLUDEPATH += .

# for Qt5 compatibility
QT += gui core
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# determine version number from git
VERSION = $$system(sh ../build/get_version.sh)
VERSTR = '\\"$${VERSION}\\"'  # place quotes around the version string
DEFINES += VER=\"$${VERSTR}\" # create a VER macro containing the version string

# determine build date (Using QMAKE_HOST here to account for cross-compilation case)
equals(QMAKE_HOST.os,Windows) {
    BUILDDATE = $$system(date /T)
} else {
    BUILDDATE = $$system(date +%a\\ %m\\/%d\\/%Y)
}
BDATESTR = '\\"$${BUILDDATE}\\"'  # place quotes around the build date string
DEFINES += BDATE=\"$${BDATESTR}\" # create a BDATE macro containing the build date string

# language logic heavily inspired by Qt Creator's
# share/qtcreator/translations/translations.pro
include(util.pri)
LANGUAGES = de \
    nb \
    sv \
    fr \
    pt \
    pl \
    ru \
    it \
    ja \
    fi \
    ar \
    el \
    da \
    tr \
    es \
    nl \
    cs \
    uk \
    br
TRANSLATIONS = $$bracketAll(LANGUAGES, qhimdtransfer_,.ts)
include(translate.pri)

# Input
HEADERS += qhimdaboutdialog.h \
    qhimdformatdialog.h \
    qhimduploaddialog.h \
    qhimdmainwindow.h \
    qhimddetection.h \
    qmdmodel.h \
    qmdtrack.h \
    qmddevice.h
FORMS += qhimdaboutdialog.ui \
    qhimdformatdialog.ui \
    qhimduploaddialog.ui \
    qhimdmainwindow.ui
SOURCES += main.cpp \
    qhimdaboutdialog.cpp \
    qhimdformatdialog.cpp \
    qhimduploaddialog.cpp \
    qhimdmainwindow.cpp \
    qhimddetection.cpp \
    qmdmodel.cpp \
    qmdtrack.cpp \
    qmddevice.cpp

win32 {
  SOURCES += qhimdwindetection.cpp
} else:mac {
  SOURCES += qhimdmacdetection.cpp
} else:unix:!mac {
  SOURCES += qhimdlinuxdetection.cpp
  QT += dbus
} else {
  SOURCES += qhimddummydetection.cpp
}

RESOURCES += icons.qrc

win32:LIBS += -lsetupapi -lcfgmgr32

SOURCES += wavefilewriter.cpp
HEADERS += wavefilewriter.h

win32:RC_FILE = qhimdtransfer.rc
mac:ICON = qhimdtransfer.icns

# Installing stuff
translations.files = $$bracketAll(LANGUAGES, qhimdtransfer_,.qm)
unix {
    INSTALLS += translations
    HACK = $$system(lrelease $$TRANSLATIONS)
    macx:translations.path = QHiMDTransfer.app/Contents/Resources/translations
    !macx:translations.path = /usr/share/qhimdtransfer/translations
}

include(../libhimd/use_libhimd.pri)
include(../libnetmd/use_libnetmd.prl)
include(../build/libusb.pri)
include(../build/libtaglib.pri)
include(../build/libmad.pri)
include(../build/libid3tag.pri)
include(../build/libz.pri)
include(../build/installunix.pri)
include(../build/common.pri)
