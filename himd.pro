TEMPLATE =subdirs
CONFIG   +=order
SUBDIRS  = libhimd himddump
!without_gui: {
  SUBDIRS += qhimdtransfer
}
