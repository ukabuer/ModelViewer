#define GLFW_INCLUDE_NONE
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "GLFW/glfw3.h"
#include "glad/glad.h"
// clang-format off
#include "sokol_gfx.h"
#include "shaders/triangle.glsl.c"
// clang-format on
#include <iostream>

using namespace std;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto window = glfwCreateWindow(640, 480, "Sokol Triangle GLFW", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    auto loader = reinterpret_cast<GLADloadproc>(&glfwGetProcAddress);
    if (!gladLoadGLLoader(loader)) {
        cerr << "Failed to load OpenGL." << endl;
        return -1;
    }

    sg_desc app_desc{};
    sg_setup(&app_desc);

    const float vertices[] = {
            // positions            // colors
            0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f};
    sg_buffer_desc buff_desc{
            .size = sizeof(vertices),
            .content = vertices,
    };
    auto vbuf = sg_make_buffer(&buff_desc);
    auto shader_desc = triangle_shader_desc();
    auto shd = sg_make_shader(shader_desc);

    sg_pipeline_desc pip_desc{};
    pip_desc.shader = shd;
    pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
    pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
    auto pip = sg_make_pipeline(&pip_desc);

    sg_bindings binds = {};
    binds.vertex_buffers[0] = vbuf;

    sg_pass_action pass_action{};
    while (!glfwWindowShouldClose(window)) {
        int cur_width, cur_height;
        glfwGetFramebufferSize(window, &cur_width, &cur_height);
        sg_begin_default_pass(&pass_action, cur_width, cur_height);
        sg_apply_pipeline(pip);
        sg_apply_bindings(&binds);
        sg_draw(0, 3, 1);
        sg_end_pass();
        sg_commit();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    sg_shutdown();
    glfwTerminate();
    return 0;
}