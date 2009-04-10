bin_PROGRAMS = himdtest
himdtest_SOURCES = himdtest.c
himdtest_LDADD = libhimd.a

lib_LIBRARIES = libhimd.a
libhimd_a_SOURCES = encryption.c \
                  himd.c \
                  mdstream.c \
                  trackindex.c
include_HEADERS = himd.h
noinst_HEADERS = himd_private.h
