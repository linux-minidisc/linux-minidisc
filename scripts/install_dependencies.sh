#!/bin/sh

set -e
set -x

unset CC
export CC
unset CXX
export CXX

case "$BUILD_TYPE" in
    mingw32|mingw64)
        docker build -t linux-minidisc-mingw-docker scripts/mingw-docker
        ;;
    linux)
        sudo apt-get update -q
        sudo apt-get install -q -y \
            qtbase5-dev qttools5-dev-tools \
            libglib2.0-dev libmad0-dev libgcrypt20-dev libusb-1.0-0-dev libid3tag0-dev libtag1-dev libjson-c-dev \
            meson ninja-build cmake
        ;;
    macos)
        brew update
        for pkg in pkg-config qt@5 mad libid3tag libtag glib libusb libusb-compat libgcrypt meson ninja cmake json-c; do
            # 2018-11-07: Fix issues related to already-existing packages
            brew install --force $pkg || brew upgrade $pkg
        done
        brew link --overwrite --force qt@5 || true
        ;;
    *)
        echo "Unset/unknown \$BUILD_TYPE: $BUILD_TYPE"
        exit 1
        ;;
esac
