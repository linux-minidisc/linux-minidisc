APP = netmd
LIB = libnetmd.so.0.1
UTILS = utilities/hex-to-char utilities/char-to-hex

PEDANTIC = -Wall -Wextra -g

LDFLAGS = `libusb-config --libs`
CFLAGS = `libusb-config --cflags` $(PEDANTIC)

all: $(APP) $(LIB)

utils: $(UTILS)

docs: libnetmd.h
	doxygen Doxyfile

clean:
	rm -fr *~ *.o *.so* *.a $(APP) $(UTILS) $(LIB) $(LIBRARY) documentation/html documentation/latex

libnetmd.a: libnetmd.o libnetmd_secure.o playercontrol.o utils.o common.o trace.o
	$(AR) rcs $@ $^

$(APP): netmd.c netmd_dev.c libnetmd.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(LIB): libnetmd.a
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^
