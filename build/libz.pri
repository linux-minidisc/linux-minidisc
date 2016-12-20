unix:!mac {
    PKGCONFIG += zlib
} else {
    LIBS += -lz
}
