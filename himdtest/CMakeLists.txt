cmake_minimum_required(VERSION 2.6)
project(himdtest)

add_executable(himdtest himdtest.c)

find_package(Glib REQUIRED)
include_directories(${Glib_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Glib_LIBRARIES})

include_directories(../libhimd)

target_link_libraries(himdtest himd ${LIBS})
