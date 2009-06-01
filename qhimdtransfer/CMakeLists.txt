project(QHiMDTransfer)
cmake_minimum_required(VERSION 2.6)
find_package( Qt4 REQUIRED )
include(${QT_USE_FILE})

find_package(Sox REQUIRED)
include_directories(${Sox_INCLUDE_DIRS})

set(qhimdtransfer_SRCS 
    main.cpp qhimdaboutdialog.cpp qhimdformatdialog.cpp qhimdmainwindow.cpp)
set(qhimdtransfer_MOC_HDRS
    qhimdmainwindow.h qhimdformatdialog.h qhimdaboutdialog.h)
set(qhimdtransfer_UIS qhimdaboutdialog.ui qhimdformatdialog.ui qhimdmainwindow.ui)
set(qhimdtransfer_QRCs icons.qrc)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

qt4_wrap_ui(QHiMDTransferUI ${qhimdtransfer_UIS})
qt4_wrap_cpp(QHiMDTransferMOC ${qhimdtransfer_MOC_HDRS})
qt4_add_resources(QHiMDTransferQRC ${qhimdtransfer_QRCs})

add_executable(QHiMDTransfer ${qhimdtransfer_SRCS} ${QHiMDTransferUI} ${QHiMDTransferMOC} ${QHiMDTransferQRC})
target_link_libraries(QHiMDTransfer ${QT_LIBRARIES} himd ${Sox_LIBRARIES})