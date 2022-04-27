#!/bin/sh

set -e
set -x

for BUILD_TYPE_HOST in i686-w64-mingw32 x86_64-w64-mingw32; do
    BUILD_TYPE_PREFIX="/usr/$BUILD_TYPE_HOST/"

    for tool in uic moc rcc; do
        sudo ln -sf "$tool" "$BUILD_TYPE_PREFIX/bin/$BUILD_TYPE_HOST-$tool"
    done
done
