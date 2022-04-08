#!/bin/sh
# Wrapper to start qmake for cross-builds in Github Actions
# This is called from .github/workflows/build.yaml with
# environment variables exported from cross_wrapper_mingw.sh

set -e -x

"${BUILD_TYPE_HOST}-qmake-qt5" "$@"
