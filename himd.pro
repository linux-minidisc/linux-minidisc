TEMPLATE =subdirs
CONFIG   +=order
SUBDIRS  = libhimd himdtest
!without_gui: {
  SUBDIRS += qhimdtransfer
}
