TEMPLATE =subdirs
CONFIG   +=order
SUBDIRS  = libnetmd libhimd netmdcli himddump
!without_gui: {
  SUBDIRS += qhimdtransfer
}
