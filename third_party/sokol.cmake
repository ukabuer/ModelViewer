include_guard()

include(FetchContent)

FetchContent_Declare(
    sokol
    GIT_REPOSITORY https://github.com/floooh/sokol.git
    GIT_TAG 1f9fd41e6dc3eedc6dff6a67218a1cd5b42011b3
    GIT_SHALLOW true
    GIT_PROGRESS TRUE
)

FetchContent_GetProperties(sokol)
if(NOT sokol_POPULATED)
  FetchContent_Populate(sokol)
endif()

FetchContent_GetProperties(
    sokol
    SOURCE_DIR sokol_src
)

add_library(sokol INTERFACE)
target_include_directories(sokol INTERFACE ${sokol_src})
