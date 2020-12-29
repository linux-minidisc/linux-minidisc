!without_gcrypt {
  # We use qt's own check to see if pkg-config can find libgcrypt, and if it can't, fall back to libgcrypt-config
  # https://github.com/qt/qtbase/blob/5.3/mkspecs/features/qt_functions.prf#L323-L336
  packagesExist(libgcrypt) {
    PKGCONFIG += libgcrypt
  } else {
    # The following is based upon the qt5 link_pkgconfig mkspec logic:
    # https://github.com/qt/qtbase/blob/5.3/mkspecs/features/link_pkgconfig.prf#L12-L27
    GCRYPT_CONFIG_CFLAGS = $$system(libgcrypt-config --cflags)

    GCRYPT_CONFIG_INCLUDEPATH = $$find(GCRYPT_CONFIG_CFLAGS, ^-I.*)
    GCRYPT_CONFIG_INCLUDEPATH ~= s/^-I(.*)/\\1/g

    GCRYPT_CONFIG_DEFINES = $$find(GCRYPT_CONFIG_CFLAGS, ^-D.*)
    GCRYPT_CONFIG_DEFINES ~= s/^-D(.*)/\\1/g

    GCRYPT_CONFIG_CFLAGS ~= s/^-[ID].*//g

    INCLUDEPATH *= $$GCRYPT_CONFIG_INCLUDEPATH
    DEFINES *= $$GCRYPT_CONFIG_DEFINES

    QMAKE_CXXFLAGS += $$GCRYPT_CONFIG_CFLAGS
    QMAKE_CFLAGS += $$GCRYPT_CONFIG_CFLAGS
    LIBS += $$system(libgcrypt-config --libs)
  }
  DEFINES += CONFIG_WITH_GCRYPT
}
else: !build_pass: message(You disabled gcrypt: No PCM and ATRAC transfer will be supported)

