unix {
    PKGCONFIG += id3tag
} else {
    LIBS += -lid3tag
}
