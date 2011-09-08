TEMPLATE=lib
TARGET  =netmd
CONFIG -= qt
CONFIG += staticlib link_pkgconfig create_prl console debug_and_release_target
DEFINES += G_DISABLE_DEPRECATED=1

PKGCONFIG += libusb-1.0
HEADERS += common.h const.h error.h libnetmd.h log.h netmd_dev.h playercontrol.h secure.h trackinformation.h utils.h
SOURCES += common.c error.c libnetmd.c log.c netmd_dev.c playercontrol.c secure.c trackinformation.c utils.c
LIBS    += -lusb-1.0 -lgcrypt
