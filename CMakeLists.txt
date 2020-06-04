cmake_minimum_required(VERSION 3.16)
project(ModelViewer)

include(cmake/utils.cmake)
option(FETCHCONTENT_QUIET "" OFF)

add_subdirectory(src)