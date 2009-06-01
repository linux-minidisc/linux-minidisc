# the QMAKE_LIBDIR thing is a workaround for a bug in qmake on mingw:
# it searches prl files for library dependencies only QMAKE_LIBDIR and
# ignores "-L" parametes in LIBS.

build_pass:CONFIG(debug,debug|release) {
 QMAKE_LIBDIR += ../libhimd/debug
 LIBS += -L../libhimd/debug
}
build_pass:CONFIG(release,debug|release) {
 QMAKE_LIBDIR += ../libhimd/release
 LIBS += -L../libhimd/release
}

# fallback if libhimd was not compiled with
# CONFIG += debug_and_release debug_and_release_target
# while I force debug_and_release_target, it is ignored in a
# just-one-kind build without debug_and_release

QMAKE_LIBDIR += ../libhimd
LIBS += -L../libhimd

INCLUDEPATH += ../libhimd
LIBS    += -lhimd
