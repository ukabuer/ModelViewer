include_guard()

find_package(OpenGL REQUIRED COMPONENTS OpenGL)

add_library(glad OBJECT ${PROJECT_SOURCE_DIR}/third_party/glad/src/glad.c)
target_link_libraries(glad PUBLIC OpenGL::OpenGL ${CMAKE_DL_LIBS})
target_include_directories(glad PUBLIC ${PROJECT_SOURCE_DIR}/third_party/glad/include)