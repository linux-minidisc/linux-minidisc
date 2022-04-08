#!/bin/sh

set -e
set -x

for BUILD_TYPE in linux-cross-mingw32 linux-cross-mingw64; do
    if [ "$BUILD_TYPE" = "linux-cross-mingw32" ]; then
        BUILD_TYPE_HOST="i686-w64-mingw32"
    else
        BUILD_TYPE_HOST="x86_64-w64-mingw32"
    fi
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
        wget -N https://sourceforge.net/projects/mad/files/libmad/0.15.1b/libmad-0.15.1b.tar.gz
        (
            rm -rf libmad-0.15.1b
            tar xvf libmad-0.15.1b.tar.gz && cd libmad-0.15.1b
            patch -p0 < ../../libmad_optimize-flags.patch
            ./configure --host=$BUILD_TYPE_HOST --prefix=$BUILD_TYPE_PREFIX
            make && sudo make install
        )
    )
done
