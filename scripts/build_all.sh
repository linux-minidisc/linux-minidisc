#!/bin/sh
# Run all builds possible on the local machine

set -e -x

case $(uname) in
    Darwin)
        env BUILD_TYPE=macos sh scripts/configure_make_package.sh
        ;;
    Linux)
        env BUILD_TYPE=linux sh scripts/configure_make_package.sh
        env BUILD_TYPE=mingw32 sh scripts/configure_make_package.sh
        env BUILD_TYPE=mingw64 sh scripts/configure_make_package.sh
        ;;
esac
