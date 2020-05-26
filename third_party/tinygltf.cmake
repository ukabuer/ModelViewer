include_guard()

add_library(tinygltf INTERFACE)
target_include_directories(tinygltf INTERFACE ${PROJECT_SOURCE_DIR}/third_party/)
target_compile_features(tinygltf INTERFACE cxx_std_11)