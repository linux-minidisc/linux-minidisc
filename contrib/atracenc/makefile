TARGET := atracenc.exe

CC := i686-w64-mingw32-gcc

LDLIBS := -lmsacm32
CFLAGS := -Os -s

all: $(TARGET)

%.exe: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDLIBS)

clean:
	$(RM) $(TARGET)

.PHONY: all clean
