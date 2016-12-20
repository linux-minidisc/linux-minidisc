TEMPLATE = app

CONFIG -= qt app_bundle
CONFIG += console link_pkgconfig link_prl

SOURCES += himdcli.c

include(../libhimd/use_libhimd.pri)
include(../build/libid3tag.pri)
include(../build/libmad.pri)
include(../build/libz.pri)
include(../build/libglib.pri)
include(../build/installunix.pri)
include(../build/common.pri)
