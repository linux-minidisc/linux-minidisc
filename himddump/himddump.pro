TEMPLATE=app
CONFIG  -= qt
CONFIG  += console link_pkgconfig link_prl
PKGCONFIG += glib-2.0 id3tag
INCLUDEPATH += ../libhimd
SOURCES += himddump.c

include(../libhimd/use_libhimd.pri)

unix:!macx {
	target.path = /usr/bin
	INSTALLS += target
}
