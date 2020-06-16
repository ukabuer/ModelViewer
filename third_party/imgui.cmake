include_guard()
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.76
    GIT_SHALLOW true
    GIT_PROGRESS TRUE
)

FetchContent_GetProperties(imgui)
if (NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif ()

FetchContent_GetProperties(
    imgui
    SOURCE_DIR imgui_src
)

add_library(
    imgui
    OBJECT
    ${imgui_src}/imgui.cpp
    ${imgui_src}/imgui_demo.cpp
    ${imgui_src}/imgui_draw.cpp
    ${imgui_src}/imgui_widgets.cpp
    ${imgui_src}/examples/imgui_impl_opengl3.cpp
    ${imgui_src}/examples/imgui_impl_glfw.cpp
)
target_include_directories(imgui PUBLIC ${imgui_src} ${imgui_src}/examples ${PROJECT_SOURCE_DIR}/third_party/glad/include)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)