CC=gcc
#CC=gcc-3.1
TAR=tar
INSTALL=install -c

UTILS = utilities/hex-to-char utilities/char-to-hex
LIBRARY = libnetmd.o
LIB = libnetmd.0.1.so
APP = netmd
PEDANTIC = -Wall

LDFLAGS = `libusb-config --libs`
CFLAGS = `libusb-config --cflags`

PKG_NAME	= unstable
DATE		= $$(date +%Y%m%d)

app: $(APP)

pedantic: PEDANTIC = -Wall -ansi
pedantic: $(APP)

utils: $(UTILS)

lib: $(LIBRARY)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -Wl,-soname,${LIB} -o ${LIB} $(LIBRARY)

docs: libnetmd.h
	doxygen Doxyfile

all: app utils

clean:
	rm -fr *~ *.o *.so* *.a $(APP) $(UTILS) $(LIB) $(LIBRARY) documentation/html documentation/latex

libnetmd.o: libnetmd.c libnetmd.h libnetmd_secure.c
	$(CC) -c ${INCLUDE} ${CFLAGS} -fPIC $<

netmd: netmd.c libnetmd.c libnetmd.h libnetmd_secure.c netmd_dev.c netmd_trace.c
	$(CC) $(CFLAGS) $(PEDANTIC) -g -o netmd netmd.c libnetmd.c libnetmd_secure.c netmd_dev.c netmd_trace.c $(LDFLAGS)

dist: clean
	cd .. && $(TAR) --exclude={CVS,cvs} -cvzf $(PKG_NAME)-$(DATE).tar.gz \
        $(PKG_NAME)

install-hotplug:
	$(INSTALL) -m 755 hotplug-netmd /etc/hotplug/usb/minidisc
	$(INSTALL) -m 644 minidisc.usermap /etc/hotplug/usb
