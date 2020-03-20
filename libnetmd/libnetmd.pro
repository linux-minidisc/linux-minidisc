TEMPLATE = lib
TARGET = netmd
CONFIG -= qt
CONFIG += staticlib link_pkgconfig create_prl console debug_and_release_target

HEADERS += common.h const.h error.h libnetmd.h log.h netmd_dev.h playercontrol.h secure.h trackinformation.h utils.h \
    libnetmd_extended.h
SOURCES += common.c error.c libnetmd.c log.c netmd_dev.c playercontrol.c secure.c trackinformation.c utils.c

include(../build/libgcrypt.pri)
include(../build/libusb.pri)
include(../build/libglib.pri)
include(../build/common.pri)
