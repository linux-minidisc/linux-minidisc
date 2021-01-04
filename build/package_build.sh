#!/bin/bash
# Build binary packages for upload to Github Releases
# 2016-12-17 Thomas Perl <m@thp.io>

set -e
set -x

HERE="$(pwd)"
PACKAGE="qhimdtransfer"
VERSION="$(sh build/get_version.sh)"

case "$BUILD_TYPE" in
    linux-cross-mingw32)
        PLATFORM="win32"
        ARCHIVE="zip"
        MINGW_BUNDLEDLLS_SEARCH_PATH=/opt/mingw32/bin:/usr/lib/gcc/i686-w64-mingw32/4.8:/usr/i686-w64-mingw32/lib
        ;;
    linux-cross-mingw64)
        PLATFORM="win64"
        ARCHIVE="zip"
        MINGW_BUNDLEDLLS_SEARCH_PATH=/opt/mingw64/bin:/usr/lib/gcc/x86_64-w64-mingw32/4.8:/usr/x86_64-w64-mingw32/lib
        ;;
    linux-native-clang)
        PLATFORM="linux-clang"
        ARCHIVE="tar"
        ;;
    linux-native-gcc)
        PLATFORM="linux-gcc"
        ARCHIVE="tar"
        ;;
    osx-native-clang)
        PLATFORM="macos"
        ARCHIVE="zip"
        ;;
    *)
        echo "Unset/unknown \$BUILD_TYPE: $BUILD_TYPE"
        exit 1
        ;;
esac

DISTNAME="${PACKAGE}-${VERSION}-${PLATFORM}"

TMP_OUT="dist-tmp/${DISTNAME}"

rm -rf dist-tmp
mkdir -p "$TMP_OUT"

# Copy documentation and platform independent stuff
cp -rpv COPYING COPYING.LIB README docs "$TMP_OUT"

case "$BUILD_TYPE" in
    linux-cross-mingw*)
        export MINGW_BUNDLEDLLS_SEARCH_PATH

        for filename in himdcli/release/himdcli.exe netmdcli/release/netmdcli.exe; do
            basename="$(basename "$filename")"
            target="$TMP_OUT/$basename"
            if test -f "$filename"; then
              cp "$filename" "$target"
            fi
            python3 build/mingw-bundledlls --copy "$target"
        done
        ;;
    linux-native-*)
        mkdir -p "$TMP_OUT/bin"
        cp -rpv himdcli/himdcli netmdcli/netmdcli "$TMP_OUT/bin"
        if test -f "qhimdtransfer/qhimdtransfer"; then
          cp -rpv qhimdtransfer/qhimdtransfer "$TMP_OUT/bin"
        fi
        ;;
    osx-native-clang)
        if test -f "qhimdtransfer/QHiMDTransfer.app"; then
          cp -rpv qhimdtransfer/QHiMDTransfer.app "$TMP_OUT"
          macdeployqt "$TMP_OUT/QHiMDTransfer.app"
        fi
        mkdir -p "$TMP_OUT/bin"
        cp -rpv himdcli/himdcli netmdcli/netmdcli "$TMP_OUT/bin"

        ;;
    *)
        echo "Unset/unknown \$BUILD_TYPE: $BUILD_TYPE"
        exit 1
        ;;
esac

rm -rf dist
mkdir -p dist

case "$ARCHIVE" in
    zip)
        (cd dist-tmp && zip -r "${HERE}/dist/${DISTNAME}.zip" *)
        ;;
    tar)
        (cd dist-tmp && tar -czvf "${HERE}/dist/${DISTNAME}.tar.gz" *)
        ;;
    *)
        echo "Unknown archive type: '$ARCHIVE'"
        exit 1
        ;;
esac

rm -rf dist-tmp
