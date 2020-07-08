find_package(glfw3 REQUIRED)
include(${PROJECT_SOURCE_DIR}/third_party/glad.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/sokol.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/tinygltf.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/Eigen.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/imgui.cmake)
include(${PROJECT_SOURCE_DIR}/third_party/portable-file-dialogs.cmake)

process_shaders(${CMAKE_CURRENT_SOURCE_DIR}/shaders)

add_executable(ModelViewer
    main.cpp
    Application.cpp
    ImGUIApplication.cpp
    Model.cpp
    render_pass/ShadowPass.cpp
    render_pass/PostProcessPass.cpp
    render_pass/SkyboxPass.cpp
    render_pass/LightingPass.cpp
    render_pass/GBufferPass.cpp
    render_pass/SSAOPass.cpp
    render_pass/BlurPass.cpp
    render_pass/utils.cpp
    Camera.cpp
    Light.cpp
    TrackballController.cpp
    utils.cpp
    Geometry.cpp)
target_link_libraries(
    ModelViewer
    PRIVATE
    sokol
    glad
    glfw
    tinygltf
    Eigen3::Eigen
    imgui
    portable_file_dialogs
    -static-libstdc++
)
target_include_directories(ModelViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ModelViewer PRIVATE cxx_std_14)
