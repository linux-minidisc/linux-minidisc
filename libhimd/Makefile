CFLAGS:=$(CFLAGS) -Wall -g `pkg-config --cflags glib-2.0`
LDLIBS:=$(LDLIBS) `pkg-config --libs glib-2.0`

OBJECTS=himd.o encryption.o trackindex.o mdstream.o

all: $(OBJECTS)

himdtest: himdtest.o $(OBJECTS)

clean:
	rm -f *.o
	rm -f himdtest

himd.o: himd.c himd.h
trackindex.o: trackindex.c himd.h
himdtest.o: himdtest.c himd.h
encryption.o: encryption.c himd.h
mdtrack.o: mdtrack.c himd.h
