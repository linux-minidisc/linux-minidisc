TEMPLATE = lib
TARGET = usbmd
CONFIG -= qt
CONFIG += staticlib link_pkgconfig create_prl console debug_and_release_target

HEADERS += libusbmd.h
SOURCES += libusbmd.c

include(../build/common.pri)
