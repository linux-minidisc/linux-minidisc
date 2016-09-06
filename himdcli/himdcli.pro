TEMPLATE=app
CONFIG  -= qt
CONFIG  += console link_pkgconfig link_prl
PKGCONFIG += glib-2.0 id3tag
INCLUDEPATH += ../libhimd
SOURCES += himdcli.c

include(../libhimd/use_libhimd.pri)

unix:!macx {
	target.path = /usr/bin
	INSTALLS += target
}

!without_mad: {
  LIBS += -lmad
  DEFINES += CONFIG_WITH_MAD
}

macx {
  CONFIG -= app_bundle
}
