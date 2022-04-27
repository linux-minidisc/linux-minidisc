#!/bin/sh

set -e
set -x

for BUILD_TYPE_HOST in i686-w64-mingw32 x86_64-w64-mingw32; do
    BUILD_TYPE_PREFIX="/usr/$BUILD_TYPE_HOST/"

    export PKG_CONFIG_PATH="$BUILD_TYPE_PREFIX/lib/pkgconfig/"
    export CFLAGS="$PFLAGS -I$BUILD_TYPE_PREFIX/include/"
    export CPPFLAGS="$CPPFLAGS -I$BUILD_TYPE_PREFIX/include/"
    export CXXFLAGS="$CXXFLAGS -I$BUILD_TYPE_PREFIX/include/"
    export LDFLAGS="$LDFLAGS -L$BUILD_TYPE_PREFIX/lib/"
    export PATH="$BUILD_TYPE_PREFIX/bin:$PATH"

    (
        mkdir -p deps && cd deps
        wget -N https://s3.amazonaws.com/json-c_releases/releases/json-c-0.16.tar.gz
        (
            rm -rf json-c-0.16
            tar xvf json-c-0.16.tar.gz && cd json-c-0.16
            cmake \
                -DCMAKE_TOOLCHAIN_FILE=../../$BUILD_TYPE_HOST.cmake \
                -DCMAKE_INSTALL_PREFIX=$BUILD_TYPE_PREFIX \
                -B build \
                .
            make -C build && sudo make -C build install
        )
    )
done
