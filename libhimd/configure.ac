AC_INIT([libhimd],[0.1])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
if [[ "$GCC" == "yes" ]]
then
  CFLAGS="$CFLAGS -Wall -W"
fi
AC_PROG_CC_C_O
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

HAVE_MAD_H=0
PKG_CHECK_MODULES([GLIB], [glib-2.0],[CFLAGS="$CFLAGS $GLIB_CFLAGS";LDFLAGS="$LDFLAGS $GLIB_LIBS"])
AC_CHECK_LIB([mad], [mad_stream_init], [HAVE_MAD_LIB=1], [HAVE_MAD_LIB=0])
if [[ "$HAVE_MAD_LIB" == "1" ]]
then
  AC_CHECK_HEADERS([mad.h],[HAVE_MAD_H=1;LDFLAGS="$LDFLAGS -lmad"])
fi
if [[ "x$HAVE_MAD_H" != "x1" ]]
then
  AC_MSG_WARN([MAD not found, MP3 reading will be unsupported])
fi

AC_CONFIG_FILES([Makefile])
AM_PATH_LIBMCRYPT([2.4.0], [CFLAGS="$CFLAGS $LIBMCRYPT_CFLAGS"
                            LDFLAGS="$LDFLAGS $LIBMCRYPT_LIBS"],
                           [AC_MSG_WARN([libmcrypt not found, WAV reading will be unsupported])])
AC_OUTPUT
