TEMPLATE = subdirs

SUBDIRS = libnetmd libhimd netmdcli himdcli

netmdcli.depends = libnetmd
himdcli.depends = libhimd

!without_gui: {
  SUBDIRS += qhimdtransfer
  qhimdtransfer.depends = libhimd libnetmd
}
