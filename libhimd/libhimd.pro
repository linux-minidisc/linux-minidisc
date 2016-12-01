TEMPLATE = lib
TARGET = himd
CONFIG -= qt
CONFIG += staticlib link_pkgconfig create_prl console debug_and_release_target

HEADERS += codecinfo.h himd.h himd_private.h sony_oma.h
SOURCES += codecinfo.c encryption.c himd.c mdstream.c trackindex.c sony_oma.c frag.c mp3tools.c

include(../build/libmad.pri)
include(../build/libgcrypt.pri)
include(../build/libglib.pri)
include(../build/common.pri)
