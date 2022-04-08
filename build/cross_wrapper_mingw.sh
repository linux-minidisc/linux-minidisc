#!/bin/sh

set -e
set -x

if [ "$BUILD_TYPE" = "linux-cross-mingw32" ]; then
    BUILD_TYPE_HOST="i686-w64-mingw32"
else
    BUILD_TYPE_HOST="x86_64-w64-mingw32"
fi
BUILD_TYPE_PREFIX="/usr/$BUILD_TYPE_HOST/"

if [ ! -z "$BUILD_TYPE_PREFIX" ]; then
    export PATH="$BUILD_TYPE_PREFIX/bin:$PATH"
    export PKG_CONFIG=$BUILD_TYPE_HOST-pkg-config
    export PKG_CONFIG_PATH=$BUILD_TYPE_PREFIX/lib/pkgconfig/
    export LDFLAGS="$LDFLAGS -L$BUILD_TYPE_PREFIX/lib/"
    BUILD_TYPE_CFLAGS="-DG_OS_WIN32 -I$BUILD_TYPE_PREFIX/include/"
    BUILD_TYPE_CFLAGS="$BUILD_TYPE_CFLAGS -I$BUILD_TYPE_PREFIX/include/libusb-1.0/"
    BUILD_TYPE_CFLAGS="$BUILD_TYPE_CFLAGS -I$BUILD_TYPE_PREFIX/include/taglib/"
    export CFLAGS="$CFLAGS $BUILD_TYPE_CFLAGS"
    export CXXFLAGS="$CXXFLAGS $BUILD_TYPE_CFLAGS"
fi

"$@"
