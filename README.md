# minidisc-ffwd: friendly fork, wip development

A friendly fork of `linux-minidisc` to more freely experiment
with things with less friction while eventually getting most
changes merged back into the upstream repository.

[Upstream](https://en.wikipedia.org/wiki/Upstream_(software_development)) repos and relationships:

```
    (o) Original Upstream Repo ("upstream")
     |  https://github.com/linux-minidisc/linux-minidisc
     |
    (o) NetMD fixes by vuori (upstream PR#62)
     |  https://github.com/vuori/linux-minidisc
     |
    (o) Downstream repo w/ fixes, used by Platium MD (upstream PR#82)
     |  https://github.com/gavinbenda/linux-minidisc
     |
  > (o) This Repo
     |  https://github.com/thp/linux-minidisc
     |
     v
```

Ideally all those changes will eventually find their way back into the
upstream repo, but for now, this faster-moving branch is where it's at.

README of the vuori fork is [here](README.vuori).
Original README of upstream is [here](README).


## Meson Build System

There is now support for the [Meson Build System](https://mesonbuild.com/),
which is a bit more flexible and modern than the old qmake (for libs, CLI).

Care was taken so that the old `qmake`-based system still works if needed.

Building with Meson is simple, it always builds out-of-source-tree (here,
`build` is used as the build folder, and `ninja` is the build system):

```
meson builddir
ninja -C builddir
```

To build `libnetmd` and `libhimd` as static libraries (e.g. so that
`netmdcli` and `himdcli` can be used standalone):

```
meson builddir -Ddefault_library=static
ninja -C builddir
```

As part of this modification, the `libnetmd` public headers do not depend
on `libusb.h` directly anymore, which helps keep things modular.

The build dependencies are now much cleaner, and Meson's dependency system
is used, which on Linux uses `pkg-config` for everything by default, which
works fine on modern distros.


## Linux NetMD Permissions

The udev rules file for Linux (so users in the "plugdev" group will have
access to NetMD devices without requiring root) is now installed in the
right place automatically when using the Meson build system.

The HAL file (`netmd/etc/20-netmd.fdi`) was removed without replacement,
as HAL is deprecated in Linux for more than 10 years now.
