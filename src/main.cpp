#define GLFW_INCLUDE_NONE
#include "Camera.hpp"
#include "GBufferPass.hpp"
#include "LightingPass.hpp"
#include "Model.hpp"
#include "PostProcessPass.hpp"
#include "ShadowPass.hpp"
#include "SkyboxPass.hpp"
#include "TrackballController.hpp"
#include "utils.hpp"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#define SOKOL_GLCORE33
#define SOKOL_IMPL
#include <sokol_gfx.h>

using namespace std;

void setup_event_callback(GLFWwindow *window, TrackballController &controller);

int main(int argc, const char *argv[]) {
  constexpr int32_t width = 1280;
  constexpr int32_t height = 720;

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  auto window =
      glfwCreateWindow(width, height, "GLTF Viewer", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  auto loader = reinterpret_cast<GLADloadproc>(&glfwGetProcAddress);
  if (!gladLoadGLLoader(loader)) {
    cerr << "Failed to load OpenGL." << endl;
    return -1;
  }

  sg_desc app_desc{};
  sg_setup(&app_desc);

  // load scene
  auto model = Model::Load(argv[1]);
  auto scene = model.gltf.scenes[model.gltf.defaultScene];
  auto bound = get_gltf_scene_bound(model.gltf, model.gltf.defaultScene);
  auto radius = abs((bound.max() - bound.min()).norm() / 2.0f);

  // setup camera
  Camera camera{};
  camera.setProjection(45.0f,
                       static_cast<float>(width) / static_cast<float>(height),
                       0.01f, 1000.0f);

  TrackballController controller{800, 600};
  controller.target = (bound.min() + bound.max()) / 2.0f;
  controller.position = controller.target;
  controller.position[2] += radius * 1.2;
  setup_event_callback(window, controller);

  ShadowPass shadow_pass{};
  auto gbuffer_pass = GBufferPass(width, height);
  auto lighting_pass = LightingPass(width, height, gbuffer_pass.position,
                                    gbuffer_pass.normal, gbuffer_pass.albedo);
  auto skybox_pass = SkyboxPass(lighting_pass.result, gbuffer_pass.depth);
  auto postprocess_pass = PostProccesPass(lighting_pass.result);

  while (!glfwWindowShouldClose(window)) {
    controller.update();
    camera.lookAt(controller.position, controller.target, controller.up);

    const Eigen::Matrix4f view_matrix = camera.getViewMatrix().inverse();
    auto &projection_matrix = camera.getCullingProjectionMatrix();
    const Eigen::Matrix4f camera_matrix = projection_matrix * view_matrix;

    shadow_pass.run(model);
    gbuffer_pass.run(model, camera_matrix);
    lighting_pass.run(controller.position);
    skybox_pass.run(camera_matrix);
    postprocess_pass.run(width, height);

    sg_commit();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  sg_shutdown();
  glfwTerminate();
  return 0;
}

void setup_event_callback(GLFWwindow *window, TrackballController &controller) {
  glfwSetWindowUserPointer(window, reinterpret_cast<void *>(&controller));
  glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scan_code,
                                int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
      return;
    }
  });
  glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y) {
    auto raw = glfwGetWindowUserPointer(window);
    auto &control = *(reinterpret_cast<TrackballController *>(raw));
    control.onMouseMove(static_cast<int>(x), static_cast<int>(y));
  });
  glfwSetScrollCallback(
      window, [](GLFWwindow *window, double x_offset, double y_offset) {
        if (y_offset == 0.0) {
          return;
        }
        auto raw = glfwGetWindowUserPointer(window);
        auto &control = *(reinterpret_cast<TrackballController *>(raw));
        control.onMouseWheelScroll(static_cast<float>(y_offset));
      });
  glfwSetMouseButtonCallback(
      window, [](GLFWwindow *window, int button, int action, int mods) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        auto raw = glfwGetWindowUserPointer(window);
        auto &control = *(reinterpret_cast<TrackballController *>(raw));
        if (action == GLFW_PRESS) {
          control.onMouseDown(GLFW_MOUSE_BUTTON_LEFT == button,
                              static_cast<int>(x), static_cast<int>(y));
        } else if (action == GLFW_RELEASE) {
          control.onMouseUp(GLFW_MOUSE_BUTTON_LEFT == button,
                            static_cast<int>(x), static_cast<int>(y));
        }
      });
}