TEMPLATE=lib
TARGET  =himd
CONFIG -= qt
CONFIG += staticlib link_pkgconfig create_prl console debug_and_release_target
DEFINES += G_DISABLE_DEPRECATED=1

!without_gcrypt: {
  LIBS += -lgcrypt
  DEFINES += CONFIG_WITH_GCRYPT
}
else: !build_pass: message(You disabled mcrypt: No PCM and ATRAC transfer will be supported)

!without_mad: {
  LIBS += -lmad
  DEFINES += CONFIG_WITH_MAD
}
else: !build_pass: message(You disabled mad: MP3 transfer will be limited)

PKGCONFIG += glib-2.0
HEADERS += codecinfo.h himd.h himd_private.h sony_oma.h
SOURCES += codecinfo.c encryption.c himd.c mdstream.c trackindex.c sony_oma.c frag.c
