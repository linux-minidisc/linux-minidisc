CFLAGS:=$(CFLAGS) -Wall -g `pkg-config --cflags glib-2.0`
LDLIBS:=$(LDLIBS) `pkg-config --libs glib-2.0`

OBJECTS=himd.o 

all: $(OBJECTS)

himdtest: himdtest.o himd.o

clean:
	rm -f *.o
	rm -f himdtest
