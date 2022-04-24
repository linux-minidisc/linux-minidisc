TEMPLATE = subdirs

SUBDIRS = libusbmd libnetmd libhimd netmdcli himdcli

libnetmd.depends = libusbmd

netmdcli.depends = libnetmd
himdcli.depends = libhimd

!without_gui {
  SUBDIRS += qhimdtransfer
  qhimdtransfer.depends = libusbmd libhimd libnetmd
}
