APP = netmd
LIB = libnetmd.so.0.1
UTILS = utilities/hex-to-char utilities/char-to-hex

PEDANTIC = -Wall -W -Wextra -Wswitch-enum -Wpointer-arith -Wcast-qual -Wcast-align
PEDANTIC += -Wconversion
PEDANTIC += -Werror
#PEDANTIC += -Wmissing-prototypes -Wstrict-prototypes
PEDANTIC += -Wwrite-strings

CFLAGS = -fshort-enums `libusb-config --cflags`
CFLAGS += $(PEDANTIC)
CFLAGS += -g

LDFLAGS = `libusb-config --libs`

all: $(APP) $(LIB)

utils: $(UTILS)

docs: libnetmd.h
	doxygen Doxyfile

clean:
	rm -fr *~ *.o *.so* *.a $(APP) $(UTILS) $(LIB) $(LIBRARY) documentation/html documentation/latex

libnetmd.a: libnetmd.o secure.o playercontrol.o utils.o common.o log.o error.o
	$(AR) rcs $@ $^

$(LIB): libnetmd.a
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^

$(APP): netmd.o netmd_dev.o libnetmd.a
