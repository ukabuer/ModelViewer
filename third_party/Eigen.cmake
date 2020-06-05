include_guard()

include(FetchContent)
FetchContent_Declare(
    eigen3
    URL https://gitlab.com/libeigen/eigen/-/archive/3.3.7/eigen-3.3.7.tar.bz2
    URL_MD5 b9e98a200d2455f06db9c661c5610496
)

FetchContent_GetProperties(eigen3)
if (NOT eigen3_POPULATED)
  FetchContent_Populate(eigen3)
endif ()

FetchContent_GetProperties(
    eigen3
    SOURCE_DIR eigen3_src
)

add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE ${eigen3_src})
add_library(Eigen3::Eigen ALIAS eigen)
