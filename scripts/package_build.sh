#!/bin/bash
# Build binary packages for upload to Github Releases
# 2016-12-17 Thomas Perl <m@thp.io>

set -e
set -x

HERE="$(pwd)"
PACKAGE="minidisc-ffwd"

get_version() {
    set +e

    GIT_VERSION="$(git describe --tags 2>/dev/null)"
    SRCDIR="$( cd "$( dirname "$0" )" && pwd )"

    if [ -n "$GIT_VERSION" ] ; then
        echo $GIT_VERSION
    elif [ -f "${SRCDIR}"/../version.txt ] ; then
        cat "${SRCDIR}"/../version.txt
    else
        echo "NO_VERSION"
    fi
}

VERSION="$(get_version)"

echo "Packaging version: ${VERSION}"

if [ -z "$BUILD_TYPE_HOST" ]; then
    echo "\$BUILD_TYPE_HOST is not set"
    exit 1
fi

BUILDDIR="build-$BUILD_TYPE_HOST"

case "$BUILD_TYPE" in
    mingw32)
        PLATFORM="win32"
        ARCHIVE="zip"
        MINGW_BUNDLEDLLS_SEARCH_PATH=$BUILDDIR:/usr/$BUILD_TYPE_HOST/bin:/usr/$BUILD_TYPE_HOST/lib:/usr/$BUILD_TYPE_HOST/sys-root/mingw/bin
        ;;
    mingw64)
        PLATFORM="win64"
        ARCHIVE="zip"
        MINGW_BUNDLEDLLS_SEARCH_PATH=$BUILDDIR:/usr/$BUILD_TYPE_HOST/bin:/usr/$BUILD_TYPE_HOST/lib:/usr/$BUILD_TYPE_HOST/sys-root/mingw/bin
        ;;
    linux)
        PLATFORM="linux"
        ARCHIVE="tar"
        ;;
    macos)
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
    mingw32|mingw64)
        export MINGW_BUNDLEDLLS_SEARCH_PATH

        (
            cd "$TMP_OUT"
            cp -v /usr/$BUILD_TYPE_HOST/sys-root/mingw/lib/qt5/plugins/platforms/qwindows.dll .
            python3 "$HERE/scripts/mingw-bundledlls" --copy qwindows.dll
            mkdir -p platforms
            mv -v qwindows.dll platforms/
        )

        for filename in himdcli.exe netmdcli.exe qhimdtransfer.exe; do
            cp "$BUILDDIR/$filename" "$TMP_OUT/$filename"
            python3 scripts/mingw-bundledlls --copy "$TMP_OUT/$filename"
        done

        mkdir -p "${TMP_OUT}/lib"
        for filename in libusbmd-0.dll libnetmd-0.dll libhimd-0.dll; do
            (cd "$TMP_OUT" && gendef "$filename")
            llvm-dlltool -D "$TMP_OUT/$filename" -d "$TMP_OUT/${filename%.dll}.def" -l "$TMP_OUT/${filename%.dll}.lib"
            mv -v "$TMP_OUT/${filename%.dll}.def" $"$TMP_OUT/${filename%.dll}.lib" "${TMP_OUT}/lib/"
        done
        cp -rpv $BUILDDIR/sysroot/usr/local/include "$TMP_OUT/"
        ;;
    linux)
        cp -rpv $BUILDDIR/sysroot/usr "$TMP_OUT/"
        ;;
    macos)
        mkdir -p $TMP_OUT/QHiMDTransfer.app/Contents/{MacOS,Resources}
        cp -rpv $BUILDDIR/qhimdtransfer "$TMP_OUT/QHiMDTransfer.app/Contents/MacOS/QHiMDTransfer"
        cp -rpv scripts/macos/*.icns "$TMP_OUT/QHiMDTransfer.app/Contents/Resources/"
        cat scripts/macos/Info.plist.in | sed -e "s/@VERSION@/$VERSION/g" > "$TMP_OUT/QHiMDTransfer.app/Contents/Info.plist"
        cp -rpv $BUILDDIR/lib*.dylib "$TMP_OUT/QHiMDTransfer.app/Contents/MacOS/"
        macdeployqt "$TMP_OUT/QHiMDTransfer.app"
        mkdir -p "$TMP_OUT/bin"
        for filename in netmdcli himdcli; do
            cp -rpv "$BUILDDIR/$filename" "$TMP_OUT/bin/$filename"
        done
        for filename in $BUILDDIR/lib{usb,net,hi}md.*dylib; do
            cp -rpv "$filename" "$TMP_OUT/bin/$(basename "$filename")"
        done
        cp -rpv $BUILDDIR/sysroot/usr/local/include "$TMP_OUT/"
        ;;
    *)
        echo "Unset/unknown \$BUILD_TYPE: $BUILD_TYPE"
        exit 1
        ;;
esac

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
