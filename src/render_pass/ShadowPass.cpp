#define SOKOL_GLCORE33
#include "ShadowPass.hpp"
#include "Camera.hpp"
#include "shaders/shadow.glsl.h"
#include <Eigen/Core>
#include <sokol_gfx.h>
#include <stack>
#include <string>
#include <unordered_set>

using namespace std;

ShadowPass::ShadowPass() {
  constexpr int size = 1024;

  sg_image_desc shadow_map_desc{};
  shadow_map_desc.render_target = true;
  shadow_map_desc.width = shadow_map_desc.height = size;
  shadow_map_desc.pixel_format = SG_PIXELFORMAT_RGBA8;
  shadow_map_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
  shadow_map_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
  shadow_map = sg_make_image(shadow_map_desc);

  sg_image_desc depth_desc = shadow_map_desc;
  depth_desc.pixel_format = SG_PIXELFORMAT_DEPTH;

  sg_pass_desc shadow_pass_desc{};
  shadow_pass_desc.color_attachments[0].image = shadow_map;
  shadow_pass_desc.depth_stencil_attachment.image = sg_make_image(depth_desc);
  pass = sg_make_pass(shadow_pass_desc);

  pass_action.colors[0].action = SG_ACTION_CLEAR;
  pass_action.colors[0].val[0] = 1.0f;
  pass_action.colors[0].val[1] = 1.0f;
  pass_action.colors[0].val[2] = 1.0f;
  pass_action.colors[0].val[3] = 1.0f;
  pass_action.depth.action = SG_ACTION_CLEAR;
  pass_action.depth.val = 1.0f;

  camera.setProjection(Camera::Projection::Orthographic, -3.0f, 3.0f, 3.0f,
                       -3.0f, 1.0f, 50.0f);
}

void ShadowPass::run(const Model &model, Light &light) {
  camera.lookAt(-light.direction * 10.0f, Eigen::Vector3f{0.0f, 0.0f, 0.0f},
                Eigen::Vector3f{0.0f, 0.0f, 1.0f});
  const Eigen::Matrix4f view_matrix = camera.getViewMatrix().inverse();
  auto &projection_matrix = camera.getCullingProjectionMatrix();

  light.matrix = projection_matrix * view_matrix;

  shadow_vs_params_t shadow_vs_params{};
  shadow_vs_params.light_space_matrix = light.matrix;

  auto scene = model.gltf.scenes[model.gltf.defaultScene];

  sg_begin_pass(pass, pass_action);
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
          bindings.index_buffer = mesh.geometry.indices;
          bindings.vertex_buffers[ATTR_shadow_vs_position] =
              mesh.geometry.positions;

          sg_apply_pipeline(mesh.shadow_pass_pipeline);
          sg_apply_bindings(bindings);
          shadow_vs_params.model = Eigen::Matrix4f::Identity();
          for (int i = 0; i < current.matrix.size(); i++) {
            shadow_vs_params.model.data()[i] = current.matrix[i];
          }

          sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_shadow_vs_params,
                            &shadow_vs_params, sizeof(shadow_vs_params));
          sg_draw(0, mesh.geometry.num, 1);
        }
      }

      processed.emplace(remain_nodes.top());
      remain_nodes.pop();
      transforms.pop();
    }
  }
  sg_end_pass();
}