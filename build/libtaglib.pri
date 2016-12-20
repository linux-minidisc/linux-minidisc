unix {
  PKGCONFIG += taglib
} else {
  LIBS += -ltag
}
