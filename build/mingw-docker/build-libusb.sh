#!/bin/sh

set -e
set -x

for BUILD_TYPE_HOST in i686-w64-mingw32 x86_64-w64-mingw32; do
    BUILD_TYPE_PREFIX="/usr/$BUILD_TYPE_HOST/"

    for tool in uic moc rcc; do
        sudo ln -sf "$tool" "$BUILD_TYPE_PREFIX/bin/$BUILD_TYPE_HOST-$tool"
    done

    export PKG_CONFIG_PATH="$BUILD_TYPE_PREFIX/lib/pkgconfig/"
    export CFLAGS="$PFLAGS -I$BUILD_TYPE_PREFIX/include/"
    export CPPFLAGS="$CPPFLAGS -I$BUILD_TYPE_PREFIX/include/"
    export CXXFLAGS="$CXXFLAGS -I$BUILD_TYPE_PREFIX/include/"
    export LDFLAGS="$LDFLAGS -L$BUILD_TYPE_PREFIX/lib/"
    export PATH="$BUILD_TYPE_PREFIX/bin:$PATH"

    (
        mkdir -p deps && cd deps
        wget -N https://github.com/libusb/libusb/releases/download/v1.0.25/libusb-1.0.25.tar.bz2
        (
            rm -rf libusb-1.0.25
            tar xvf libusb-1.0.25.tar.bz2 && cd libusb-1.0.25
            ./configure --host=$BUILD_TYPE_HOST --prefix=$BUILD_TYPE_PREFIX
            make && sudo make install
        )
    )
done
