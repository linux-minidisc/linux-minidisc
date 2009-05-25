cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

project(HiMD)
add_subdirectory(libhimd)
add_subdirectory(QHiMDTransfer)
