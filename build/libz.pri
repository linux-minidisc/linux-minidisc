unix:!mac {
    PKGCONFIG += zlib
} else {
    # Windows builds with QT4 will get confused by the link order of zlib,
    # so we use QMAKE_LFLAGS instead to force it to be dead last.
    # https://stackoverflow.com/a/31483107
    !greaterThan(QT_MAJOR_VERSION, 4):!unix {
        QMAKE_LFLAGS += -lz
    } else {
        LIBS += -lz
    }
}
