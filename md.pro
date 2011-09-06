TEMPLATE =subdirs
CONFIG   +=order
SUBDIRS  = libnetmd libhimd netmdcli himdcli
!without_gui: {
  SUBDIRS += qhimdtransfer
}
