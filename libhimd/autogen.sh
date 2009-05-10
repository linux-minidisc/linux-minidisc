#!/bin/sh
libtoolize
aclocal $ACLOCAL_FLAGS
autoconf
autoheader
automake --add-missing
./configure "$@"
