# http://stackoverflow.com/a/17578151/1047040
QMAKE_CXXFLAGS += $$(CXXFLAGS)
QMAKE_CFLAGS += $$(CFLAGS)
QMAKE_LFLAGS += $$(LDFLAGS)

macx {
  # Dependencies from Homebrew are put here
  INCLUDEPATH += /usr/local/include
  LIBS += -L/usr/local/lib
}
