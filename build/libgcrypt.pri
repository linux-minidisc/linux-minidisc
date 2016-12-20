!without_gcrypt {
  LIBS += -lgcrypt
  DEFINES += CONFIG_WITH_GCRYPT
}
else: !build_pass: message(You disabled gcrypt: No PCM and ATRAC transfer will be supported)

