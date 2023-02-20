#!/bin/sh

set -e
set -x

build_and_package() {
    meson build-$BUILD_TYPE_HOST -Dwith_gui=true "$@"
    # Non-parallel builds for easier-to-read logs in CI
    ninja -C build-$BUILD_TYPE_HOST -j1
    env DESTDIR=$(pwd)/build-$BUILD_TYPE_HOST/sysroot/ ninja -C build-$BUILD_TYPE_HOST -j1 install
    bash scripts/package_build.sh
}

case "$BUILD_TYPE" in
    mingw*)
        if [ "$INCEPTION" = "running-in-docker" ]; then
            case "$BUILD_TYPE" in
                mingw32)
                    BUILD_TYPE_HOST="i686-w64-mingw32"
                    ;;
                mingw64)
                    BUILD_TYPE_HOST="x86_64-w64-mingw32"
                    ;;
                *)
                    echo "Unknown \$BUILD_TYPE: $BUILD_TYPE"
                    exit 1
                    ;;
            esac

            export BUILD_TYPE_HOST

            BUILD_TYPE_PREFIX="/usr/$BUILD_TYPE_HOST/"

            if [ ! -z "$BUILD_TYPE_PREFIX" ]; then
                export PATH="$BUILD_TYPE_PREFIX/bin:$PATH"
                export PKG_CONFIG=$BUILD_TYPE_HOST-pkg-config
                export PKG_CONFIG_PATH=$BUILD_TYPE_PREFIX/lib/pkgconfig/
                export LDFLAGS="$LDFLAGS -L$BUILD_TYPE_PREFIX/lib/"
                BUILD_TYPE_CFLAGS="-I$BUILD_TYPE_PREFIX/include/"
                BUILD_TYPE_CFLAGS="$BUILD_TYPE_CFLAGS -I$BUILD_TYPE_PREFIX/include/libusb-1.0/"
                BUILD_TYPE_CFLAGS="$BUILD_TYPE_CFLAGS -I$BUILD_TYPE_PREFIX/sys-root/mingw/include/taglib/"
                export CFLAGS="$CFLAGS $BUILD_TYPE_CFLAGS"
                export CXXFLAGS="$CXXFLAGS $BUILD_TYPE_CFLAGS"
            fi

            # Added "-Dwerror=false" temporarily for Qt error:
            # error: 'uint qHash(const QKeySequence&, uint)' redeclared without
            # dllimport attribute: previous dllimport ignored [-Werror=attributes]
            build_and_package -Dtarget_os=windows --cross-file "scripts/mingw-cross-meson/$BUILD_TYPE_HOST.txt" -Dwerror=false
        else
            export INCEPTION=running-in-docker
            docker run \
                -e INCEPTION -e BUILD_TYPE \
                --user "$(id -u):$(id -g)" \
                -v $(pwd):/build/ \
                --rm linux-minidisc-mingw-docker \
                sh scripts/configure_make_package.sh
        fi
        ;;
    macos)
        export BUILD_TYPE_HOST="$(gcc -dumpmachine)"
        build_and_package -Dtarget_os=macos
        ;;
    linux)
        export BUILD_TYPE_HOST="$(gcc -dumpmachine)"
        build_and_package -Dtarget_os=linux
        ;;
    *)
        echo "Unknown \$BUILD_TYPE: $BUILD_TYPE"
        ;;
esac
