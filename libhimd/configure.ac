AC_INIT([libhimd],[0.1])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

AC_PROG_RANLIB
AC_PROG_CC
AC_PROG_CC_C_O
PKG_CHECK_MODULES([GLIB], [glib-2.0],[CFLAGS="$CFLAGS $GLIB_CFLAGS";LDFLAGS="$LDFLAGS $GLIB_LIBS"])
PKG_CHECK_MODULES([MAD], [mad],[CFLAGS="$CFLAGS $MAD_CFLAGS"
                                LDFLAGS="$LDFLAGS $MAD_LIBS"
                                AC_DEFINE(HAVE_MAD,[1],[Whether the MAD package is installed])],
                               [AC_MSG_WARN([MAD not found, MP3 reading will be unsupported])])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
