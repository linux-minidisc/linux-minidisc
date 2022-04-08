#!/bin/sh

set -e
set -x

unset CC
export CC
unset CXX
export CXX

case "$BUILD_TYPE" in
    linux-cross-mingw*)
        if [ "$BUILD_TYPE" = "linux-cross-mingw32" ]; then
            BUILD_TYPE_HOST="i686-w64-mingw32"
        else
            BUILD_TYPE_HOST="x86_64-w64-mingw32"
        fi
        BUILD_TYPE_PREFIX="/usr/$BUILD_TYPE_HOST/"

        sudo add-apt-repository --yes ppa:tobydox/mingw-w64
        sudo apt-get update -q || true
        sudo apt-get install -q -f -y mingw-w64 mingw-w64-tools \
            qt5base-mingw-w64 glib2-mingw-w64 libz-mingw-w64-dev

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
                tar xvf libmad-0.15.1b.tar.gz && cd libmad-0.15.1b
                patch -p0 < ../../build/libmad_optimize-flags.patch
                ./configure --host=$BUILD_TYPE_HOST --prefix=$BUILD_TYPE_PREFIX
                make && sudo make install
            )

            wget -N https://sourceforge.net/projects/mad/files/libid3tag/0.15.1b/libid3tag-0.15.1b.tar.gz
            (
                tar xvf libid3tag-0.15.1b.tar.gz && cd libid3tag-0.15.1b
                ./configure --host=$BUILD_TYPE_HOST --prefix=$BUILD_TYPE_PREFIX
                make && sudo make install
            )

            wget -N https://gnupg.org/ftp/gcrypt/libgpg-error/libgpg-error-1.45.tar.bz2
            (
                tar xvf libgpg-error-1.45.tar.bz2 && cd libgpg-error-1.45
                ./configure --host=$BUILD_TYPE_HOST --prefix=$BUILD_TYPE_PREFIX
                make && sudo make install
            )

            wget -N https://gnupg.org/ftp/gcrypt/libgcrypt/libgcrypt-1.10.1.tar.bz2
            (
                tar xvf libgcrypt-1.10.1.tar.bz2 && cd libgcrypt-1.10.1
                ./configure --host=$BUILD_TYPE_HOST --prefix=$BUILD_TYPE_PREFIX
                make && sudo make install
            )

            wget -N https://taglib.github.io/releases/taglib-1.11.tar.gz
            (
                tar xvf taglib-1.11.tar.gz && cd taglib-1.11
                mkdir build && cd build
                cmake \
                    -DCMAKE_TOOLCHAIN_FILE=$(pwd)/../../../build/toolchain-$BUILD_TYPE_HOST.cmake \
                    -DCMAKE_INSTALL_PREFIX=$BUILD_TYPE_PREFIX \
                    -DBUILD_SHARED_LIBS=ON \
                    -DBUILD_BINDINGS=OFF \
                    ..
                make && sudo make install
            )
        )
        ;;
    linux-native-*)
        sudo apt-get update -q
        sudo apt-get install -q -y qtbase5-dev qttools5-dev-tools libglib2.0-dev libmad0-dev libgcrypt20-dev libusb-1.0-0-dev libid3tag0-dev libtag1-dev
        ;;
    osx-native-*)
        brew update
        for pkg in pkg-config qt5 mad libid3tag libtag glib libusb libusb-compat libgcrypt; do
            # 2018-11-07: Fix issues related to already-existing packages
            brew install --force $pkg || brew upgrade $pkg
        done
        brew link --force qt5
        ;;
    *)
        echo "Unset/unknown \$BUILD_TYPE: $BUILD_TYPE"
        exit 1
        ;;
esac
