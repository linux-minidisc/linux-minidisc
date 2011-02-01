TEMPLATE = app
CONFIG += link_prl \
    link_pkgconfig
TARGET = qhimdtransfer
DEPENDPATH += .
INCLUDEPATH += .

# determine version number from git
VERSION = $$system(git describe --always --long)
#    !isEmpty(VERSION){
#      VERSION = 0.0.1-$${VERSION}
#    }

VERSTR = '\\"$${VERSION}\\"'  # place quotes around the version string
DEFINES += VER=\"$${VERSTR}\" # create a VER macro containing the version string

# determine build date
unix:BUILDDATE = $$system(date +%a\\ %m\\/%d\\/%Y)
win32:BUILDDATE = $$system(date /T)

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
    cs
TRANSLATIONS = $$bracketAll(LANGUAGES, qhimdtransfer_,.ts)
include(translate.pri)

# Input
HEADERS += qhimdaboutdialog.h \
    qhimdformatdialog.h \
    qhimduploaddialog.h \
    qhimdmainwindow.h \
    qhimdmodel.h \
    qhimddetection.h
FORMS += qhimdaboutdialog.ui \
    qhimdformatdialog.ui \
    qhimduploaddialog.ui \
    qhimdmainwindow.ui
SOURCES += main.cpp \
    qhimdaboutdialog.cpp \
    qhimdformatdialog.cpp \
    qhimduploaddialog.cpp \
    qhimdmainwindow.cpp \
    qhimdmodel.cpp \
    qhimddetection.cpp
win32:SOURCES += qhimdwindetection.cpp
else:SOURCES += qhimddummydetection.cpp
RESOURCES += icons.qrc
PKGCONFIG += sox \
    taglib
win32:LIBS += -lsetupapi \
    -lcfgmgr32
win32:RC_FILE = qhimdtransfer.rc
mac:ICON = qhimdtransfer.icns

# Mixed case version of the target name for operating systems on which
# this is convention.
win32:TARGET = QHiMDTransfer
mac:TARGET = QHiMDTransfer
include(../libhimd/use_libhimd.pri)

# Installing stuff
translations.files = $$bracketAll(LANGUAGES, qhimdtransfer_,.qm)
unix: { 
    INSTALLS += translations
    HACK = $$system(lrelease $$TRANSLATIONS)
    macx:translations.path = QHiMDTransfer.app/Contents/Resources/translations
    !macx:translations.path = /usr/share/qhimdtransfer/translations
}
unix:!macx { 
    target.path = /usr/bin
    INSTALLS += target
}
