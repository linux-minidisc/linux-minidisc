TEMPLATE =subdirs
CONFIG   +=order
SUBDIRS  = libnetmd libhimd himddump
!without_gui: {
  SUBDIRS += qhimdtransfer
}
