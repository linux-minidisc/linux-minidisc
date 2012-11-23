#/bin/sh
# get_version.sh - simple script to determine version number

GIT_VERSION="$(git describe --always --long 2>/dev/null)"

if [ -n "$GIT_VERSION" ] ; then
    echo $GIT_VERSION
elif [ -f VERSION ] ; then
    echo $(< VERSION)
else
    echo "NO_VERSION"
fi
