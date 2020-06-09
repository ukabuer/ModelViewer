#define GLFW_INCLUDE_NONE
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "Camera.hpp"
#include "TrackballController.hpp"
#include "utils.hpp"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include <stack>
#include <string>
#include <unordered_set>
// clang-format off
#include "Model.hpp"
#include "shaders/render.glsl.h"
#include "shaders/postprocess.glsl.h"
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

  TrackballController controller{800, 600};
  controller.target = (bound.min() + bound.max()) / 2.0f;
  controller.position = controller.target;
  controller.position[2] += radius * 1.2;
  setup_event_callback(window, controller);

  // geometry pass
  sg_image_desc image_desc{};
  image_desc.render_target = true;
  image_desc.width = width;
  image_desc.height = height;
  image_desc.min_filter = SG_FILTER_LINEAR;
  image_desc.mag_filter = SG_FILTER_LINEAR;
  image_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
  image_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
  image_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;

  sg_image_desc depth_desc = image_desc;
  depth_desc.pixel_format = SG_PIXELFORMAT_DEPTH;

  sg_pass_desc geometry_pass_desc{};
  geometry_pass_desc.color_attachments[0].image = sg_make_image(image_desc);
  geometry_pass_desc.color_attachments[1].image = sg_make_image(image_desc);
  geometry_pass_desc.color_attachments[2].image = sg_make_image(image_desc);
  geometry_pass_desc.depth_stencil_attachment.image = sg_make_image(depth_desc);
  auto geometry_pass = sg_make_pass(geometry_pass_desc);

  sg_pass_action geometry_pass_action{};
  for (int i = 0; i < 3; i++) {
    geometry_pass_action.colors[i].action = SG_ACTION_CLEAR;
    for (auto &j : geometry_pass_action.colors[i].val) {
      j = 0.0f;
    }
  }

  sg_bindings geometry_pass_binds = {};

  geometry_vs_params_t geometry_vs_params{};

  // postprocess pass
  const vector<float> screen_quad = {
      -1.0, -1.0, 0.0, 0.0, 1.0, -1.0, 1.0, 0.0,
      -1.0, 1.0,  0.0, 1.0, 1.0, 1.0,  1.0, 1.0,
  };
  sg_buffer_desc screen_quad_desc = {};
  screen_quad_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
  screen_quad_desc.size = screen_quad.size() * sizeof(float);
  screen_quad_desc.content = screen_quad.data();
  auto screen_quad_buffer = sg_make_buffer(screen_quad_desc);

  sg_pass_action postprocess_pass_action{};
  postprocess_pass_action.colors[0].action = SG_ACTION_CLEAR;
  for (auto &i : postprocess_pass_action.colors[0].val) {
    i = 0.0f;
  }

  sg_pipeline_desc postprocess_pipeline_desc{};
  postprocess_pipeline_desc.shader = sg_make_shader(postprocess_shader_desc());
  postprocess_pipeline_desc.layout.attrs[ATTR_postprocess_vs_position].format =
      SG_VERTEXFORMAT_FLOAT2;
  postprocess_pipeline_desc.layout.attrs[ATTR_postprocess_vs_uv].format =
      SG_VERTEXFORMAT_FLOAT2;
  postprocess_pipeline_desc.layout.attrs[ATTR_postprocess_vs_uv].offset =
      2 * sizeof(float);
  postprocess_pipeline_desc.layout.buffers[0].stride = 4 * sizeof(float);
  postprocess_pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
  auto postprocess_pipeline = sg_make_pipeline(postprocess_pipeline_desc);

  sg_bindings postprocess_pass_binds = {};
  postprocess_pass_binds.fs_images[SLOT_g_world_pos] =
      geometry_pass_desc.color_attachments[0].image;
  postprocess_pass_binds.fs_images[SLOT_g_normal] =
      geometry_pass_desc.color_attachments[1].image;
  postprocess_pass_binds.fs_images[SLOT_g_albedo] =
      geometry_pass_desc.color_attachments[2].image;
  postprocess_pass_binds.vertex_buffers[0] = screen_quad_buffer;

  postprocess_fs_params_t postprocess_fs_params{};

  while (!glfwWindowShouldClose(window)) {
    controller.update();
    camera.lookAt(controller.position, controller.target, controller.up);

    const Eigen::Matrix4f viewMatrix = camera.getViewMatrix().inverse();
    auto &projectionMatrix = camera.getCullingProjectionMatrix();
    postprocess_fs_params.view_pos = controller.position;
    geometry_vs_params.camera = projectionMatrix * viewMatrix;

    sg_begin_pass(geometry_pass, geometry_pass_action);
    auto gltf_node_idxs = scene.nodes;
    unordered_set<int> processed{};
    if (!gltf_node_idxs.empty()) {
      stack<int> remain_nodes{};
      stack<Eigen::Matrix4f> transforms{};
      remain_nodes.emplace(gltf_node_idxs[0]);

      Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
      for (auto i = 0; i < 16; i++) {
        transform.data()[i] = model.gltf.nodes[remain_nodes.top()].matrix[i];
      }
      transforms.emplace(move(transform));
      while (!remain_nodes.empty()) {
        auto &current_idx = remain_nodes.top();
        auto &current = model.gltf.nodes[current_idx];

        auto has_new = false;
        for (auto child_idx : current.children) {
          if (processed.find(child_idx) == processed.end()) {
            remain_nodes.emplace(child_idx);
            auto &child_node = model.gltf.nodes[child_idx];
            auto &parent_transform = transforms.top();
            Eigen::Matrix4f child_transform = Eigen::Matrix4f::Identity();
            for (auto i = 0; i < 16; i++) {
              child_transform.data()[i] = child_node.matrix[i];
            }
            transforms.emplace(parent_transform * child_transform);
            has_new = true;
            break;
          }
        }

        if (has_new) {
          continue;
        }

        if (current.mesh != -1) {
          auto &gltf_mesh = model.gltf.meshes[current.mesh];
          for (uint32_t idx = 0; idx < gltf_mesh.primitives.size(); idx++) {
            string id = to_string(current.mesh) + "-" + to_string(idx);
            auto mesh_pos = model.meshes.find(id);
            if (mesh_pos == model.meshes.end()) {
              continue;
            }

            auto &mesh = mesh_pos->second;
            geometry_pass_binds.index_buffer = mesh.geometry.indices;
            geometry_pass_binds.vertex_buffers[ATTR_vs_position] =
                mesh.geometry.positions;
            geometry_pass_binds.vertex_buffers[ATTR_vs_normal] =
                mesh.geometry.normals;
            geometry_pass_binds.vertex_buffers[ATTR_vs_uv] = mesh.geometry.uvs;
            geometry_pass_binds.fs_images[SLOT_albedo] = mesh.albedo;

            sg_apply_pipeline(mesh.pipeline);
            sg_apply_bindings(geometry_pass_binds);
            geometry_vs_params.model = Eigen::Matrix4f::Identity();
            for (int i = 0; i < current.matrix.size(); i++) {
              geometry_vs_params.model.data()[i] = current.matrix[i];
            }

            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_geometry_vs_params,
                              &geometry_vs_params,
                              sizeof(geometry_vs_params_t));
            sg_draw(0, mesh.geometry.num, 1);
          }
        }

        processed.emplace(remain_nodes.top());
        remain_nodes.pop();
        transforms.pop();
      }
    }
    sg_end_pass();

    sg_begin_default_pass(postprocess_pass_action, width, height);
    sg_apply_pipeline(postprocess_pipeline);
    sg_apply_bindings(postprocess_pass_binds);
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_postprocess_fs_params,
                      &postprocess_fs_params, sizeof(postprocess_fs_params_t));
    sg_draw(0, 4, 1);
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