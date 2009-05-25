project(QHiMDTransfer)
cmake_minimum_required(VERSION 2.6)
find_package( Qt4 REQUIRED )
include(${QT_USE_FILE})

set(qhimdtransfer_SRCS 
    main.cpp qhimdaboutdialog.cpp qhimdformatdialog.cpp qhimdmainwindow.cpp)
set(qhimdtransfer_MOC_HDRS
    qhimdmainwindow.h qhimdformatdialog.h qhimdaboutdialog.h)
set(qhimdtransfer_UIS qhimdaboutdialog.ui qhimdformatdialog.ui qhimdmainwindow.ui)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

qt4_wrap_ui(QHiMDTransferUI ${qhimdtransfer_UIS})
qt4_wrap_cpp(QHiMDTransferMOC ${qhimdtransfer_MOC_HDRS})

add_executable(QHiMDTransfer ${qhimdtransfer_SRCS} ${QHiMDTransferUI} ${QHiMDTransferMOC})
target_link_libraries(QHiMDTransfer ${QT_LIBRARIES} himd)
