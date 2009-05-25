cmake_minimum_required(VERSION 2.6)
project(libhimd)

#add_library(himd SHARED encryption.c himd.c mdstream.c trackindex.c)
add_library(himd encryption.c himd.c mdstream.c trackindex.c)
add_executable(himdtest himdtest.c)

find_package(Glib REQUIRED)
find_package(Mad)
find_package(Mcrypt)
include_directories(${Glib_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Glib_LIBRARIES} ${Mad_LIBRARIES} ${Mcrypt_LIBRARIES})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

target_link_libraries(himd ${LIBS})

target_link_libraries(himdtest himd)
