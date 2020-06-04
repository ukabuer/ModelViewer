#define GLFW_INCLUDE_NONE
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "Camera.hpp"
#include <glad/glad.h>
#include "Scene.hpp"
#include "TrackballController.hpp"
#include "utils.hpp"
#include <GLFW/glfw3.h>
// clang-format off
#include "shaders/triangle.glsl.c"
// clang-format on
#include <iostream>

#include <string>

using namespace std;

void setup_event_callback(GLFWwindow *window, TrackballController &controller);

int main() {
  constexpr int32_t width = 640;
  constexpr int32_t height = 480;

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  auto window =
      glfwCreateWindow(width, height, "Sokol Triangle GLFW", nullptr, nullptr);
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
  auto model = Scene::Load("/home/ukabuer/Downloads/models/Box.gltf");
  auto bound = get_gltf_scene_bound(model.model, model.model.defaultScene);
  auto scene = model.model.scenes[model.model.defaultScene];

  // setup camera
  Camera camera{};
  camera.setProjection(45.0f,
                       static_cast<float>(width) / static_cast<float>(height),
                       0.01f, 1000.0f);

  TrackballController controller{640, 480};
  controller.target = (bound.min() + bound.max()) / 2.0f;
  controller.position = controller.target;
  auto radius = (bound.max() - bound.min()).norm();
  controller.position[2] += 5.0f;
  setup_event_callback(window, controller);

  auto shader_desc = triangle_shader_desc();
  auto shader = sg_make_shader(shader_desc);

  sg_pass_action pass_action{};
  sg_bindings binds = {};

  sg_pipeline_desc pip_desc{};
  pip_desc.shader = shader;
  pip_desc.index_type =SG_INDEXTYPE_UINT16;
  pip_desc.layout.attrs[0].buffer_index = 0;
  pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
  auto pipeline = sg_make_pipeline(&pip_desc);
  while (!glfwWindowShouldClose(window)) {
    controller.update();
    camera.lookAt(controller.position, controller.target, controller.up);

    const Eigen::Matrix4f viewMatrix = camera.getViewMatrix().inverse();
    auto &projectionMatrix = camera.getCullingProjectionMatrix();
    Eigen::Matrix4f view_matrix = projectionMatrix * viewMatrix;

    sg_begin_default_pass(&pass_action, width, height);
    sg_apply_pipeline(pipeline);

    auto nodes = scene.nodes;
    while (!nodes.empty()) {
      auto node_idx = nodes.back();
      nodes.pop_back();
      auto &node = model.model.nodes[node_idx];
      if (!node.children.empty()) {
        nodes.insert(nodes.end(), node.children.begin(), node.children.end());
      }

      if (node.mesh < 0) {
        continue;
      }

      auto &mesh = model.model.meshes[node.mesh];
      for (auto &primitive : mesh.primitives) {
        auto num = 0;
        if (primitive.indices >= 0) {
          auto &accessor = model.model.accessors[primitive.indices];
          binds.index_buffer = model.buffers[accessor.bufferView];
          num = accessor.count;
        }
        auto position_pos = primitive.attributes.find("POSITION");
        auto &accessor = model.model.accessors[position_pos->second];

        binds.vertex_buffers[0] = model.buffers[accessor.bufferView];

        sg_apply_bindings(&binds);
        Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
        for (int i = 0; i < node.matrix.size(); i++) {
          transform.data()[i] = node.matrix[i];
        }

        std::array<Eigen::Matrix4f, 2> vs_params = {transform, view_matrix};

        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, vs_params.data(), sizeof(Eigen::Matrix4f) * 2);

        sg_draw(0, num, 1);
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