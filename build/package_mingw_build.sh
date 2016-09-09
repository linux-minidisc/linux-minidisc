#!/bin/sh

set -e
set -x

mkdir out
cp himdcli/release/himdcli.exe \
   netmdcli/release/netmdcli.exe \
   qhimdtransfer/release/QHiMDTransfer.exe \
   /opt/mingw32/bin/*.dll \
   /usr/lib/gcc/i686-w64-mingw32/5.3-win32/libgcc_s_sjlj-1.dll \
   /usr/lib/gcc/i686-w64-mingw32/5.3-win32/libstdc++-6.dll \
     out/
