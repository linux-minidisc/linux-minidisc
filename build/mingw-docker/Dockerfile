FROM fedora:latest

RUN dnf install -y \
    wget patch make bzip2 diffutils binutils \
    mingw32-gcc                mingw64-gcc \
    mingw32-gcc-c++            mingw64-gcc-c++ \
    mingw32-libgcrypt          mingw64-libgcrypt \
    mingw32-libid3tag          mingw64-libid3tag \
    mingw32-taglib             mingw64-taglib \
    mingw32-qt5-qtbase-devel   mingw64-qt5-qtbase-devel \
    mingw32-qt5-qttools-tools  mingw64-qt5-qttools-tools

COPY libmad_optimize-flags.patch /root/
COPY build-libmad.sh /root/
RUN cd /root/ && bash -e -x /root/build-libmad.sh

COPY build-libusb.sh /root/
RUN cd /root/ && bash -e -x /root/build-libusb.sh

RUN ln -sv \
    /usr/x86_64-w64-mingw32/bin/qt5/lrelease \
    /usr/x86_64-w64-mingw32/sys-root/mingw/bin/lrelease

RUN ln -sv \
    /usr/i686-w64-mingw32/bin/qt5/lrelease \
    /usr/i686-w64-mingw32/sys-root/mingw/bin/lrelease

RUN mkdir -pv /build
WORKDIR /build
