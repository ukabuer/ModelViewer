find_package(glfw3 REQUIRED)
include(${PROJECT_SOURCE_DIR}/third_party/glad.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/sokol.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/tinygltf.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/Eigen.cmake)

find_program(
    SOKOL_SHDC sokol-shdc
    HINTS ${SOKOL_SHDC_DIR}
)
if (NOT SOKOL_SHDC)
  message(FATAL_ERROR "Cannot find sokol-shdc, please set \"SOKOL_SHDC_DIR\" to a directory containing it.")
endif ()

set(SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/render.glsl)
add_custom_command(
    OUTPUT
    ${SHADER_SRC}.h
    DEPENDS
    ${SHADER_SRC}
    COMMAND
    ${SOKOL_SHDC} --input ${SHADER_SRC} --output ${SHADER_SRC}.h --slang glsl330
    COMMENT
    "Generating shader headers with sokol-shdc"
    VERBATIM
)
add_custom_target(shaders DEPENDS ${SHADER_SRC}.h)

add_executable(ModelViewer main.cpp Model.cpp Camera.cpp TrackballController.cpp utils.cpp)
target_link_libraries(ModelViewer PRIVATE sokol glad glfw tinygltf Eigen3::Eigen -static-libstdc++)
target_include_directories(ModelViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ModelViewer PRIVATE cxx_std_14)
add_dependencies(ModelViewer shaders)
