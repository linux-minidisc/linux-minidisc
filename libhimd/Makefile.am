bin_PROGRAMS = himdtest
himdtest_SOURCES = himdtest.c
himdtest_LDADD = libhimd.la

lib_LTLIBRARIES = libhimd.la
libhimd_la_SOURCES = encryption.c \
                  himd.c \
                  mdstream.c \
                  trackindex.c
include_HEADERS = himd.h
noinst_HEADERS = himd_private.h
