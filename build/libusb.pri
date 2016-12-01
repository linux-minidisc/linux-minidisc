unix {
  PKGCONFIG += libusb-1.0
} else {
  LIBS += -lusb-1.0
}
