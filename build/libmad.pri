!without_mad {
  DEFINES += CONFIG_WITH_MAD
  unix {
    PKGCONFIG += mad
  } else {
    LIBS += -lmad
  }
} else:!build_pass {
  message(You disabled mad: MP3 transfer will be limited)
}
