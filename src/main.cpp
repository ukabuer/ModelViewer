#define GLFW_INCLUDE_NONE
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "Camera.hpp"
#include "TrackballController.hpp"
#include "utils.hpp"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <string>
// clang-format off
#include "Model.hpp"
#include "shaders/render.glsl.h"
// clang-format on

using namespace std;

void setup_event_callback(GLFWwindow *window, TrackballController &controller);

int main(int argc, const char *argv[]) {
  constexpr int32_t width = 640;
  constexpr int32_t height = 480;

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

  TrackballController controller{640, 480};
  controller.target = (bound.min() + bound.max()) / 2.0f;
  controller.position = controller.target;
  controller.position[2] += radius;
  setup_event_callback(window, controller);

  sg_pass_action pass_action{};
  pass_action.colors[0].action = SG_ACTION_CLEAR;
  for (auto &i : pass_action.colors[0].val) {
    i = 0.0f;
  }
  sg_bindings binds = {};
  vs_params_t vs_params{};
  fs_params_t fs_params{};
  while (!glfwWindowShouldClose(window)) {
    controller.update();
    camera.lookAt(controller.position, controller.target, controller.up);

    const Eigen::Matrix4f viewMatrix = camera.getViewMatrix().inverse();
    auto &projectionMatrix = camera.getCullingProjectionMatrix();
    fs_params.view_pos = controller.position;
    vs_params.camera = projectionMatrix * viewMatrix;

    sg_begin_default_pass(&pass_action, width, height);
    auto gltf_nodes = scene.nodes;
    while (!gltf_nodes.empty()) {
      auto node_idx = gltf_nodes.back();
      gltf_nodes.pop_back();

      auto &gltf_node = model.gltf.nodes[node_idx];
      if (!gltf_node.children.empty()) {
        gltf_nodes.insert(gltf_nodes.end(), gltf_node.children.begin(),
                          gltf_node.children.end());
      }

      if (gltf_node.mesh == -1) {
        continue;
      }

      auto &gltf_mesh = model.gltf.meshes[gltf_node.mesh];
      for (uint32_t idx = 0; idx < gltf_mesh.primitives.size(); idx++) {
        string id = to_string(gltf_node.mesh) + "-" + to_string(idx);
        auto mesh_pos = model.meshes.find(id);
        if (mesh_pos == model.meshes.end()) {
          continue;
        }

        auto &mesh = mesh_pos->second;
        binds.index_buffer = mesh.geometry.indices;
        binds.vertex_buffers[0] = mesh.geometry.positions;
        binds.vertex_buffers[1] = mesh.geometry.normals;
        binds.vertex_buffers[2] = mesh.geometry.uvs;
        binds.fs_images[SLOT_albedo] = mesh.albedo;

        sg_apply_pipeline(mesh.pipeline);
        sg_apply_bindings(&binds);
        vs_params.model = Eigen::Matrix4f::Identity();
        for (int i = 0; i < gltf_node.matrix.size(); i++) {
          vs_params.model.data()[i] = gltf_node.matrix[i];
        }

        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params,
                          sizeof(vs_params_t));
        sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fs_params,
                          sizeof(fs_params_t));
        sg_draw(0, mesh.geometry.num, 1);
      }
    }
    sg_end_pass();
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