find_package(glfw3 REQUIRED)
include(${PROJECT_SOURCE_DIR}/third_party/glad.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/sokol.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/tinygltf.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/Eigen.cmake)

process_shaders(${CMAKE_CURRENT_SOURCE_DIR}/shaders)

add_executable(ModelViewer
    main.cpp
    Model.cpp
    ShadowPass.cpp
    PostProcessPass.cpp
    SkyboxPass.cpp
    LightingPass.cpp
    GBufferPass.cpp
    Camera.cpp
    TrackballController.cpp
    utils.cpp
)
target_link_libraries(ModelViewer PRIVATE sokol glad glfw tinygltf Eigen3::Eigen -static-libstdc++)
target_include_directories(ModelViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ModelViewer PRIVATE cxx_std_14)
