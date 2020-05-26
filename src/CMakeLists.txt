find_package(glfw3 REQUIRED)
include(${PROJECT_SOURCE_DIR}/third_party/glad.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/sokol.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/tinygltf.cmake)

add_executable(ModelViewer main.cpp)
target_link_libraries(ModelViewer PRIVATE sokol glad glfw tinygltf -static-libstdc++)
target_include_directories(ModelViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ModelViewer PRIVATE cxx_std_14)
