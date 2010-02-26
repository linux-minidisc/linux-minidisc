TEMPLATE = app
CONFIG += link_prl link_pkgconfig
TARGET = qhimdtransfer
DEPENDPATH += .
INCLUDEPATH += .

# language logic heavily inspired by Qt Creator's
# share/qtcreator/translations/translations.pro
include(translate.pri)
LANGUAGES = de nb sv fr pt pl ru it ja fi ar el
TRANSLATIONS = $$bracketAll(LANGUAGES, $$PWD/qhimdtransfer_,.ts)

# Input

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
mac: ICON = qhimdtransfer.icns

# Mixed case version of the target name for operating systems on which
# this is convention.
win32: TARGET=QHiMDTransfer
mac: TARGET=QHiMDTransfer

include(../libhimd/use_libhimd.pri)

# Installing stuff

translations.files = $$bracketAll(LANGUAGES, $$OUT_PWD/qhimdtransfer_,.qm)

unix: !macx {
	target.path = /usr/bin
	translations.path = /usr/share/qhimdtransfer/translations
	INSTALLS += target translations
}

macx {
	translations.path = myapp.app/Contents/Resources/translations
	INSTALLS += translations
}
