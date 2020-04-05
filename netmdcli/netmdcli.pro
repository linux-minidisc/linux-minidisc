TEMPLATE = app
CONFIG -= qt app_bundle
CONFIG += console link_pkgconfig link_prl
SOURCES += netmdcli.c

include(../libnetmd/use_libnetmd.prl)
include(../build/libjson-c.pri)
include(../build/libusb.pri)
include(../build/installunix.pri)
include(../build/common.pri)
