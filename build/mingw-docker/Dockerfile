FROM fedora:latest

RUN dnf install -y \
    wget patch make bzip2 diffutils binutils \
    mingw32-gcc                mingw64-gcc \
    mingw32-gcc-c++            mingw64-gcc-c++ \
    mingw32-libgcrypt          mingw64-libgcrypt \
    mingw32-libid3tag          mingw64-libid3tag \
    mingw32-taglib             mingw64-taglib \
    mingw32-qt5-qtbase-devel   mingw64-qt5-qtbase-devel

COPY libmad_optimize-flags.patch /root/
COPY build-libmad.sh /root/
RUN cd /root/ && bash -e -x /root/build-libmad.sh

COPY build-libusb.sh /root/
RUN cd /root/ && bash -e -x /root/build-libusb.sh

RUN mkdir -pv /build
WORKDIR /build
