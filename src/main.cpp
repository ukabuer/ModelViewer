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
#include <tinygltf/stb_image.h>
#include <unordered_set>
// clang-format off
#include "Model.hpp"
#include "shaders/gbuffer.glsl.h"
#include "shaders/shading.glsl.h"
#include "shaders/skybox.glsl.h"
#include "shaders/postprocess.glsl.h"
// clang-format on

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

  // G-buffer pass
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

  sg_pass_desc gbuffer_pass_desc{};
  gbuffer_pass_desc.color_attachments[0].image = sg_make_image(image_desc);
  gbuffer_pass_desc.color_attachments[1].image = sg_make_image(image_desc);
  gbuffer_pass_desc.color_attachments[2].image = sg_make_image(image_desc);
  gbuffer_pass_desc.depth_stencil_attachment.image = sg_make_image(depth_desc);
  auto gbuffer_pass = sg_make_pass(gbuffer_pass_desc);

  sg_pass_action gbuffer_pass_action{};
  for (int i = 0; i < 3; i++) {
    gbuffer_pass_action.colors[i].action = SG_ACTION_CLEAR;
  }
  gbuffer_pass_action.depth.action = SG_ACTION_CLEAR;
  gbuffer_pass_action.depth.val = 1.0f;

  sg_bindings gbuffer_pass_binds = {};

  gbuffer_vs_params_t gbuffer_vs_params{};

  // shading pass
  sg_pass_desc shading_pass_desc{};
  shading_pass_desc.color_attachments[0].image = sg_make_image(image_desc);
  auto shading_pass = sg_make_pass(shading_pass_desc);

  const vector<float> screen_quad = {
      -1.0, -1.0, 0.0, 0.0, 1.0, -1.0, 1.0, 0.0,
      -1.0, 1.0,  0.0, 1.0, 1.0, 1.0,  1.0, 1.0,
  };

  sg_buffer_desc screen_quad_desc = {};
  screen_quad_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
  screen_quad_desc.size = screen_quad.size() * sizeof(float);
  screen_quad_desc.content = screen_quad.data();
  auto screen_quad_buffer = sg_make_buffer(screen_quad_desc);

  sg_pass_action shading_pass_action{};
  shading_pass_action.colors[0].action = SG_ACTION_CLEAR;

  sg_pipeline_desc shading_pipeline_desc{};
  shading_pipeline_desc.shader = sg_make_shader(shading_shader_desc());
  shading_pipeline_desc.layout.attrs[ATTR_shading_vs_position].format =
      SG_VERTEXFORMAT_FLOAT2;
  shading_pipeline_desc.layout.attrs[ATTR_shading_vs_uv].format =
      SG_VERTEXFORMAT_FLOAT2;
  shading_pipeline_desc.layout.attrs[ATTR_shading_vs_uv].offset =
      2 * sizeof(float);
  shading_pipeline_desc.layout.buffers[0].stride = 4 * sizeof(float);
  shading_pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
  shading_pipeline_desc.blend.depth_format = SG_PIXELFORMAT_NONE;
  shading_pipeline_desc.blend.color_attachment_count = 1;
  shading_pipeline_desc.blend.color_format = SG_PIXELFORMAT_RGBA32F;
  auto shading_pipeline = sg_make_pipeline(shading_pipeline_desc);

  sg_bindings shading_pass_binds = {};
  shading_pass_binds.fs_images[SLOT_g_world_pos] =
      gbuffer_pass_desc.color_attachments[0].image;
  shading_pass_binds.fs_images[SLOT_g_normal] =
      gbuffer_pass_desc.color_attachments[1].image;
  shading_pass_binds.fs_images[SLOT_g_albedo] =
      gbuffer_pass_desc.color_attachments[2].image;
  shading_pass_binds.vertex_buffers[0] = screen_quad_buffer;

  shading_fs_params_t shading_fs_params{};

  // skybox pass
  sg_pass_desc skybox_pass_desc{};
  skybox_pass_desc.color_attachments[0] =
      shading_pass_desc.color_attachments[0];
  skybox_pass_desc.depth_stencil_attachment =
      gbuffer_pass_desc.depth_stencil_attachment;
  auto skybox_pass = sg_make_pass(skybox_pass_desc);

  sg_pass_action skybox_pass_action{};
  skybox_pass_action.colors[0].action = SG_ACTION_DONTCARE;
  skybox_pass_action.depth.action = SG_ACTION_DONTCARE;
  skybox_pass_action.stencil.action = SG_ACTION_DONTCARE;

  vector<string> faces = {
      "assets/textures/skybox/right.jpg", "assets/textures/skybox/left.jpg",
      "assets/textures/skybox/top.jpg",   "assets/textures/skybox/bottom.jpg",
      "assets/textures/skybox/front.jpg", "assets/textures/skybox/back.jpg",
  };
  sg_image_desc skybox_texture_desc{};
  skybox_texture_desc.type = SG_IMAGETYPE_CUBE;
  skybox_texture_desc.width = 0;
  skybox_texture_desc.height = 0;
  auto image_channels = 0;
  for (uint32_t i = 0; i < 6; i++) {
    auto data = stbi_load(faces[i].c_str(), &skybox_texture_desc.width,
                          &skybox_texture_desc.height, &image_channels, 4);
    skybox_texture_desc.content.subimage[i][0].ptr = data;
    skybox_texture_desc.content.subimage[i][0].size =
        skybox_texture_desc.width * skybox_texture_desc.width * image_channels;
    //    stbi_image_free(data);
  }

  // clang-format off
  vector<float> skybox_positions = {
      // top
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f,
       1.0f,  1.0f,  1.0f,
       1.0f,  1.0f, -1.0f,
      // bottom
      -1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f, -1.0f,  1.0f,
      // left
      -1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      // right
       1.0f,  1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,
       1.0f, -1.0f, -1.0f,
       1.0f,  1.0f, -1.0f,
      // front
      -1.0f,  1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,
      1.0f,  -1.0f,  1.0f,
      1.0f,   1.0f,  1.0f,
      // back
       1.0f,  1.0f, -1.0f,
       1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
  };
  // clang-format on
  vector<uint16_t> skybox_indices = {
      0,  2,  1,  0,  3,  2,  4,  6,  5,  4,  7,  6,  8,  10, 9,  8,  11, 10,
      12, 14, 13, 12, 15, 14, 16, 18, 17, 16, 19, 18, 20, 22, 21, 20, 23, 22,
  };
  sg_buffer_desc skybox_vertices_buffer_desc{};
  skybox_vertices_buffer_desc.content = skybox_positions.data();
  skybox_vertices_buffer_desc.size = skybox_positions.size() * sizeof(float);

  sg_buffer_desc skybox_indices_buffer_desc{};
  skybox_indices_buffer_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
  skybox_indices_buffer_desc.content = skybox_indices.data();
  skybox_indices_buffer_desc.size = skybox_indices.size() * sizeof(uint16_t);

  sg_bindings skybox_bindings{};
  skybox_bindings.vertex_buffers[0] =
      sg_make_buffer(skybox_vertices_buffer_desc);
  skybox_bindings.index_buffer = sg_make_buffer(skybox_indices_buffer_desc);
  skybox_bindings.fs_images[SLOT_skybox_cube] =
      sg_make_image(skybox_texture_desc);

  sg_pipeline_desc skybox_pipeline_desc{};
  skybox_pipeline_desc.shader = sg_make_shader(skybox_shader_desc());
  skybox_pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
  skybox_pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
  skybox_pipeline_desc.layout.attrs[ATTR_skybox_vs_position].format =
      SG_VERTEXFORMAT_FLOAT3;
  skybox_pipeline_desc.layout.attrs[ATTR_skybox_vs_position].buffer_index = 0;
  skybox_pipeline_desc.blend.depth_format = SG_PIXELFORMAT_DEPTH;
  skybox_pipeline_desc.blend.color_format = SG_PIXELFORMAT_RGBA32F;
  skybox_pipeline_desc.depth_stencil.depth_compare_func =
      SG_COMPAREFUNC_LESS_EQUAL;
  auto skybox_pipeline = sg_make_pipeline(skybox_pipeline_desc);

  // postprocess pass
  sg_pass_action postprocess_pass_action{};
  postprocess_pass_action.colors[0].action = SG_ACTION_CLEAR;

  sg_pipeline_desc postprocess_pipeline_desc = shading_pipeline_desc;
  postprocess_pipeline_desc.shader = sg_make_shader(postprocess_shader_desc());
  postprocess_pipeline_desc.blend.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
  postprocess_pipeline_desc.blend.color_format = SG_PIXELFORMAT_RGBA8;
  auto postprocess_pipeline = sg_make_pipeline(postprocess_pipeline_desc);

  sg_bindings postprocess_binds = {};
  postprocess_binds.fs_images[SLOT_rendered] =
      skybox_pass_desc.color_attachments[0].image;
  postprocess_binds.vertex_buffers[0] = screen_quad_buffer;

  while (!glfwWindowShouldClose(window)) {
    controller.update();
    camera.lookAt(controller.position, controller.target, controller.up);

    const Eigen::Matrix4f view_matrix = camera.getViewMatrix().inverse();
    auto &projectionMatrix = camera.getCullingProjectionMatrix();
    gbuffer_vs_params.camera = projectionMatrix * view_matrix;
    shading_fs_params.view_pos = controller.position;

    sg_begin_pass(gbuffer_pass, gbuffer_pass_action);
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
            gbuffer_pass_binds.index_buffer = mesh.geometry.indices;
            gbuffer_pass_binds.vertex_buffers[ATTR_gbuffer_vs_position] =
                mesh.geometry.positions;
            gbuffer_pass_binds.vertex_buffers[ATTR_gbuffer_vs_normal] =
                mesh.geometry.normals;
            gbuffer_pass_binds.vertex_buffers[ATTR_gbuffer_vs_uv] =
                mesh.geometry.uvs;
            gbuffer_pass_binds.fs_images[SLOT_albedo] = mesh.albedo;

            sg_apply_pipeline(mesh.pipeline);
            sg_apply_bindings(gbuffer_pass_binds);
            gbuffer_vs_params.model = Eigen::Matrix4f::Identity();
            for (int i = 0; i < current.matrix.size(); i++) {
              gbuffer_vs_params.model.data()[i] = current.matrix[i];
            }

            sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_gbuffer_vs_params,
                              &gbuffer_vs_params, sizeof(gbuffer_vs_params_t));
            sg_draw(0, mesh.geometry.num, 1);
          }
        }

        processed.emplace(remain_nodes.top());
        remain_nodes.pop();
        transforms.pop();
      }
    }
    sg_end_pass();

    sg_begin_pass(shading_pass, shading_pass_action);
    sg_apply_pipeline(shading_pipeline);
    sg_apply_bindings(shading_pass_binds);
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_shading_fs_params,
                      &shading_fs_params, sizeof(shading_fs_params_t));
    sg_draw(0, 4, 1);
    sg_end_pass();

    sg_begin_pass(skybox_pass, skybox_pass_action);
    sg_apply_pipeline(skybox_pipeline);
    sg_apply_bindings(skybox_bindings);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_skybox_vs_params,
                      &gbuffer_vs_params.camera, sizeof(Eigen::Matrix4f));
    sg_draw(0, skybox_indices.size(), 1);
    sg_end_pass();

    sg_begin_default_pass(postprocess_pass_action, width, height);
    sg_apply_pipeline(postprocess_pipeline);
    sg_apply_bindings(postprocess_binds);
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