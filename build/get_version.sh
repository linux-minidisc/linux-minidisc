#/bin/sh
# get_version.sh - simple script to determine version number

GIT_VERSION="$(git describe --always --long 2>/dev/null)"
SRCDIR="$( cd "$( dirname "$0" )" && pwd )"

if [ -n "$GIT_VERSION" ] ; then
    echo $GIT_VERSION
elif [ -f "${SRCDIR}"/../VERSION ] ; then
    cat "${SRCDIR}"/../VERSION
else
    echo "NO_VERSION"
fi
