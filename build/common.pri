# http://stackoverflow.com/a/17578151/1047040
QMAKE_CXXFLAGS += $$(CXXFLAGS)
QMAKE_CFLAGS += $$(CFLAGS)
QMAKE_LFLAGS += $$(LDFLAGS)

DEFINES += HIMD_QMAKE_BUILD_SYSTEM

macx {
  # Dependencies from Homebrew are put here
  INCLUDEPATH += /usr/local/include
  INCLUDEPATH += /opt/homebrew/include
  LIBS += -L/usr/local/lib
  LIBS += -L/opt/homebrew/lib
}
